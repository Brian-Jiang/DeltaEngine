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
    yield
    delta_mcp_server._close_connection()


def _noop_connect():
    return None


def test_command_single_response_strips_wire_fields(monkeypatch):
    def fake_read_line(timeout_s):
        return {
            "phase": "result",
            "request_id": "1",
            "ok": True,
            "commandType": "EditorCommand_CreateGameObject",
            "objectId": "obj-123",
        }

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


def test_command_error_result_passed_through(monkeypatch):
    def fake_read_line(timeout_s):
        return {"request_id": "1", "ok": False, "error": "bad"}

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_line", fake_read_line)

    result = delta_mcp_server._send_mcp_command(
        {"type": "command", "system": "common", "command": "SaveProject", "params": {}}
    )

    assert result == {"ok": False, "error": "bad"}


def test_command_timeout_returns_error(monkeypatch):
    closed = []

    def fake_read_matching_line(request_id, timeout_s):
        return None

    def fake_close():
        closed.append(True)

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_matching_line", fake_read_matching_line)
    monkeypatch.setattr(delta_mcp_server, "_close_connection", fake_close)

    result = delta_mcp_server._send_mcp_command(
        {"type": "command", "system": "scene", "command": "CreateGameObject", "params": {}}
    )

    assert result["ok"] is False
    assert "Timed out" in result["error"]
    assert closed == []


def test_stray_nonmatching_line_is_skipped(monkeypatch):
    queue = [
        {"request_id": "orphan-99", "ok": True, "objectId": "stale"},
        {"request_id": "1", "ok": True, "objectId": "fresh"},
    ]

    def fake_read_line(timeout_s):
        return queue.pop(0)

    monkeypatch.setattr(delta_mcp_server, "_ensure_connected", _noop_connect)
    monkeypatch.setattr(delta_mcp_server, "_read_line", fake_read_line)

    result = delta_mcp_server._send_mcp_command(
        {"type": "command", "system": "scene", "command": "CreateGameObject", "params": {}}
    )

    assert result == {"ok": True, "objectId": "fresh"}


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
