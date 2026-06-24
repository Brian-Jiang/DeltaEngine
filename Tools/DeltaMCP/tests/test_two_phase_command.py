import pytest

import delta_mcp_server


@pytest.fixture(autouse=True)
def reset_mcp_state():
    class _FakeSock:
        def sendall(self, data):
            pass

        def settimeout(self, timeout):
            pass

        def recv(self, size):
            return b""

        def close(self):
            pass

    delta_mcp_server._sock = _FakeSock()
    delta_mcp_server._buf = ""
    delta_mcp_server._request_id_counter = 0
    delta_mcp_server._orphan_buffer = []
    yield
    delta_mcp_server._close_connection()


def _noop_connect():
    return None


def test_accept_only_expects_result_false(monkeypatch):
    calls = []

    def fake_read_line(timeout_s):
        calls.append(timeout_s)
        return {
            "phase": "accept",
            "request_id": "1",
            "ok": True,
            "expects_result": False,
            "command": "SaveProject",
            "queued": True,
        }

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_line", fake_read_line)

    result = delta_mcp_server._send_mcp_command(
        {"type": "command", "system": "common", "command": "SaveProject", "params": {}}
    )

    assert result == {"ok": True, "command": "SaveProject"}
    assert len(calls) == 1


def test_full_two_phase_success(monkeypatch):
    queue = [
        {
            "phase": "accept",
            "request_id": "1",
            "ok": True,
            "expects_result": True,
            "command": "CreateGameObject",
            "queued": True,
        },
        {
            "phase": "result",
            "request_id": "1",
            "ok": True,
            "commandType": "EditorCommand_CreateGameObject",
            "objectId": "obj-123",
        },
    ]

    def fake_read_line(timeout_s):
        return queue.pop(0)

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_line", fake_read_line)

    result = delta_mcp_server._send_mcp_command(
        {
            "type": "command",
            "system": "scene",
            "command": "CreateGameObject",
            "params": {"name": "Cube"},
        }
    )

    assert result == {
        "ok": True,
        "commandType": "EditorCommand_CreateGameObject",
        "objectId": "obj-123",
    }
    assert "phase" not in result
    assert "request_id" not in result
    assert "queued" not in result
    assert "expects_result" not in result


def test_timeout_fallback(monkeypatch):
    accept = {
        "phase": "accept",
        "request_id": "1",
        "ok": True,
        "expects_result": True,
        "command": "CreateGameObject",
        "queued": True,
    }
    state = {"phase": "accept"}

    def fake_read_matching_line(phase, request_id, timeout_s):
        if state["phase"] == "accept":
            state["phase"] = "result"
            return accept
        return None

    closed = []

    def fake_close():
        closed.append(True)

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_matching_line", fake_read_matching_line)
    monkeypatch.setattr(delta_mcp_server, "_close_connection", fake_close)

    result = delta_mcp_server._send_mcp_command(
        {
            "type": "command",
            "system": "scene",
            "command": "CreateGameObject",
            "params": {"name": "Cube"},
        }
    )

    assert result["ok"] is True
    assert result["command"] == "CreateGameObject"
    assert result["execution_pending"] is True
    assert "timeout_message" in result
    assert closed == []


def test_orphan_result_discarded(monkeypatch):
    queue = [
        {
            "phase": "result",
            "request_id": "orphan-99",
            "ok": True,
            "objectId": "stale",
        },
        {
            "phase": "accept",
            "request_id": "1",
            "ok": True,
            "expects_result": True,
            "command": "CreateGameObject",
            "queued": True,
        },
        {
            "phase": "result",
            "request_id": "1",
            "ok": True,
            "objectId": "fresh",
        },
    ]

    def fake_read_line(timeout_s):
        return queue.pop(0)

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_line", fake_read_line)

    result = delta_mcp_server._send_mcp_command(
        {
            "type": "command",
            "system": "scene",
            "command": "CreateGameObject",
            "params": {"name": "Cube"},
        }
    )

    assert result == {"ok": True, "objectId": "fresh"}
    assert len(delta_mcp_server._orphan_buffer) == 1
    assert delta_mcp_server._orphan_buffer[0]["request_id"] == "orphan-99"


def test_query_unchanged(monkeypatch):
    calls = []

    def fake_read_line(timeout_s):
        calls.append(timeout_s)
        return {"ok": True, "objects": []}

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_line", fake_read_line)

    result = delta_mcp_server._send_query(
        {"type": "query", "system": "scene", "query": "game_objects", "params": {}}
    )

    assert result == {"ok": True, "objects": []}
    assert len(calls) == 1
