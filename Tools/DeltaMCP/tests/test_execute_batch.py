import delta_mcp_server


def test_valid_op_forwarded(monkeypatch):
    sent = []

    def fake_send(op):
        sent.append(op)
        return {"ok": True}

    monkeypatch.setattr(delta_mcp_server, "_send_mcp_command", fake_send)

    result = delta_mcp_server.execute_batch(
        [{"type": "command", "system": "scene",
          "command": "CreateGameObject", "params": {"name": "Sun"}}]
    )
    assert result["ok"] is True
    assert len(sent) == 1


def test_invalid_op_not_forwarded(monkeypatch):
    sent = []

    def fake_send(op):
        sent.append(op)
        return {"ok": True}

    monkeypatch.setattr(delta_mcp_server, "_send_mcp_command", fake_send)

    result = delta_mcp_server.execute_batch(
        [{"type": "command", "system": "scene",
          "command": "DoesNotExist", "params": {}}]
    )
    assert result["ok"] is False
    assert sent == []
    assert result["results"][0]["stage"] == "operation"


def test_mixed_batch(monkeypatch):
    sent = []

    def fake_send(op):
        sent.append(op)
        return {"ok": True}

    monkeypatch.setattr(delta_mcp_server, "_send_mcp_command", fake_send)

    result = delta_mcp_server.execute_batch([
        {"type": "command", "system": "scene",
         "command": "CreateGameObject", "params": {"name": "A"}},
        {"type": "command", "system": "scene",
         "command": "Bogus", "params": {}},
    ])
    assert result["ok"] is False
    assert len(sent) == 1
    assert result["results"][0]["ok"] is True
    assert result["results"][1]["ok"] is False


def test_batch_three_mixed_query_and_commands(monkeypatch):
    query_calls = []
    command_calls = []

    def fake_query(op):
        query_calls.append(op)
        return {"ok": True, "objects": [{"objectId": "go-1"}]}

    def fake_command(op):
        command_calls.append(op)
        if op.get("command") == "SaveProject":
            return {"ok": True, "command": "SaveProject"}
        return {
            "ok": True,
            "commandType": "EditorCommand_SetProperty",
            "objectId": "go-1",
        }

    monkeypatch.setattr(delta_mcp_server, "_send_query", fake_query)
    monkeypatch.setattr(delta_mcp_server, "_send_mcp_command", fake_command)

    result = delta_mcp_server.execute_batch([
        {"type": "query", "system": "scene", "query": "game_objects", "params": {}},
        {"type": "command", "system": "common", "command": "SaveProject", "params": {}},
        {
            "type": "command",
            "system": "common",
            "command": "SetProperty",
            "params": {
                "objectId": "go-1",
                "propertyName": "m_name",
                "valueAfter": "Renamed",
            },
        },
    ])

    assert result["ok"] is True
    assert len(result["results"]) == 3
    assert result["results"][0] == {"ok": True, "objects": [{"objectId": "go-1"}]}
    assert result["results"][1] == {"ok": True, "command": "SaveProject"}
    assert result["results"][2] == {
        "ok": True,
        "commandType": "EditorCommand_SetProperty",
        "objectId": "go-1",
    }
    assert len(query_calls) == 1
    assert len(command_calls) == 2
    assert query_calls[0]["query"] == "game_objects"
    assert command_calls[0]["command"] == "SaveProject"
    assert command_calls[1]["command"] == "SetProperty"


def test_local_meta_query_bypasses_validation(monkeypatch):
    sent = []

    def fake_send(op):
        sent.append(op)
        return {"ok": True}

    monkeypatch.setattr(delta_mcp_server, "_send_query", fake_send)

    result = delta_mcp_server.execute_batch(
        [{"type": "query", "system": "meta", "query": "active_systems", "params": {}}]
    )
    assert result["ok"] is True
    assert len(sent) == 0
