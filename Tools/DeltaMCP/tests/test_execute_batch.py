import delta_mcp_server


def test_valid_op_forwarded(monkeypatch):
    sent = []

    def fake_send(op):
        sent.append(op)
        return {"ok": True}

    monkeypatch.setattr(delta_mcp_server, "_send_command", fake_send)

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

    monkeypatch.setattr(delta_mcp_server, "_send_command", fake_send)

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

    monkeypatch.setattr(delta_mcp_server, "_send_command", fake_send)

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


def test_local_meta_query_bypasses_validation(monkeypatch):
    sent = []

    def fake_send(op):
        sent.append(op)
        return {"ok": True}

    monkeypatch.setattr(delta_mcp_server, "_send_command", fake_send)

    result = delta_mcp_server.execute_batch(
        [{"type": "query", "system": "meta", "query": "active_systems", "params": {}}]
    )
    assert result["ok"] is True
    assert len(sent) == 1
