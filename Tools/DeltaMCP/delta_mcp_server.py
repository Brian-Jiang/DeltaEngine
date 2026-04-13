import json
import pathlib
import socket

from mcp.server.fastmcp import FastMCP


def _find_repo_root() -> pathlib.Path:
    here = pathlib.Path(__file__).resolve()
    for parent in [here, *here.parents]:
        if (parent / "CMakePresets.json").exists():
            return parent
    raise RuntimeError(
        "Could not locate repo root (CMakePresets.json not found). "
        "Ensure delta_mcp_server.py is inside the repo tree."
    )


def _load_schemas() -> dict:
    schemas_dir = pathlib.Path(__file__).resolve().parent / "Schemas"
    systems: dict = {}
    for path in sorted(schemas_dir.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        if "system" in data:
            systems[data["system"]] = data
    return {"systems": systems}


_SCHEMAS = _load_schemas()


def _list_operations() -> dict:
    index = {}
    for name, schema in _SCHEMAS["systems"].items():
        index[name] = {
            "operations": list(schema.get("operations", {}).keys()),
            "commands": list(schema.get("commands", {}).keys()),
        }
    return {"ok": True, "systems": index}


def _describe_operations(requested: list[dict]) -> dict:
    ops_result: dict = {}
    cmds_result: dict = {}
    errors: list[str] = []
    for entry in requested:
        sys_name = entry.get("system", "")
        if "command" in entry:
            cmd_name = entry["command"]
            key = f"{sys_name}/{cmd_name}" if sys_name else cmd_name
            sys_schema = _SCHEMAS["systems"].get(sys_name) if sys_name else None
            schema = sys_schema.get("commands", {}).get(cmd_name) if sys_schema else None
            if schema is not None:
                cmds_result[key] = schema
            else:
                errors.append(f"Unknown command: {key}")
        elif "operation" in entry:
            op_name = entry["operation"]
            key = f"{sys_name}/{op_name}"
            sys_schema = _SCHEMAS["systems"].get(sys_name)
            if sys_schema and op_name in sys_schema.get("operations", {}):
                ops_result[key] = sys_schema["operations"][op_name]
            else:
                errors.append(f"Unknown operation: {key}")
    result: dict = {"ok": True, "operations": ops_result, "commands": cmds_result}
    if errors:
        result["warnings"] = errors
    return result


_LOCAL_META_OPS = {"list_operations", "describe_operations",
                   "capabilities", "active_systems"}


def _handle_local_meta(payload: dict) -> dict:
    op = payload.get("operation")
    params = payload.get("params", {})
    if op == "list_operations":
        return _list_operations()
    if op == "describe_operations":
        return _describe_operations(params.get("operations", []))
    if op == "capabilities":
        sf = params.get("system_filter", "")
        if sf:
            sys_schema = _SCHEMAS["systems"].get(sf)
            return {
                "ok": True,
                "systems": {sf: sys_schema} if sys_schema else {},
            }
        return {"ok": True, **_SCHEMAS}
    if op == "active_systems":
        stub = {"animation", "timeline", "cloth", "physics"}
        all_systems = list(_SCHEMAS["systems"].keys())
        return {
            "ok": True,
            "active": [s for s in all_systems if s not in stub],
            "stub_only": [s for s in all_systems if s in stub],
            "note": "stub_only systems return not-yet-implemented from C++",
        }
    return {"ok": False, "error": f"Unknown meta operation: {op}"}


mcp_server = FastMCP("DeltaEditor")

_sock: socket.socket | None = None
_buf: str = ""


def _send_command(payload: dict) -> dict:
    global _sock, _buf

    if (
        payload.get("type") == "query"
        and payload.get("system") == "meta"
        and payload.get("operation") in _LOCAL_META_OPS
    ):
        return _handle_local_meta(payload)

    try:
        if _sock is None:
            port_file = (
                _find_repo_root() / "Intermediate" / "EditorState" / "DeltaEditor.port"
            )
            if not port_file.exists():
                return {
                    "ok": False,
                    "error": "DeltaEditor is not running. "
                    "Launch the editor and try again.",
                }
            _sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            _sock.settimeout(5.0)
            _sock.connect(("127.0.0.1", int(port_file.read_text().strip())))
            _buf = ""
        line = json.dumps(payload) + "\n"
        _sock.sendall(line.encode())
        while "\n" not in _buf:
            chunk = _sock.recv(4096).decode()
            if not chunk:
                raise RuntimeError("Editor disconnected")
            _buf += chunk
        response_line, _buf = _buf.split("\n", 1)
        return json.loads(response_line)
    except Exception as e:
        if _sock:
            try:
                _sock.close()
            except Exception:
                pass
        _sock = None
        _buf = ""
        return {"ok": False, "error": str(e)}


@mcp_server.tool()
def list_operations() -> dict:
    """
    Returns a lightweight index of all available systems, their query
    operations, and their commands.

    Call this first to discover what the editor exposes. The response has:
      "systems": {
        "<system>": {
          "operations": ["<operation>", ...],
          "commands":   ["<command>", ...]
        },
        ...
      }

    Use describe_operations to get full parameter schemas before calling
    execute_batch.
    """
    return _list_operations()


@mcp_server.tool()
def describe_operations(operations: list[dict]) -> dict:
    """
    Returns full parameter schemas for specific operations or commands.

    Each entry in `operations` is one of:
      {"system": "<system>", "operation": "<op>"}      -- for query operations
      {"system": "<system>", "command": "<command>"}   -- for commands

    Example:
      [
        {"system": "scene", "operation": "game_objects"},
        {"system": "scene", "command": "CreateGameObject"}
      ]

    The response contains:
      "operations": { "<system>/<op>": { description, params }, ... }
      "commands":   { "<system>/<command>": { description, params, returns }, ... }
    """
    return _describe_operations(operations)


@mcp_server.tool()
def execute_batch(operations: list[dict]) -> dict:
    """
    Execute one or more operations sequentially and return all results.

    Each operation is a dict with a "type" field:

    QUERY — read state from the editor:
      {
        "type": "query",
        "system": "<system>",
        "operation": "<op>",
        "params": { ... }
      }
      Example:
        {"type":"query","system":"scene","operation":"game_objects","params":{}}

    COMMAND — mutate scene state (each command is its own undo entry):
      {
        "type": "command",
        "system": "<system>",
        "command": "<command>",
        "params": { ... }
      }
      Example:
        {"type":"command","system":"scene","command":"CreateGameObject","params":{"name":"Sun"}}

    Workflow:
      1. Call list_operations to discover systems, operations, and commands.
      2. Call describe_operations to get required params for what you need.
      3. Query scene state to obtain objectIds if you need to target existing objects.
      4. Call execute_batch with your queries and/or commands.

    Returns:
      { "ok": <true if all succeeded>, "results": [ <one result per operation> ] }
    """
    results = []
    for op in operations:
        results.append(_send_command(op))
    all_ok = all(r.get("ok", False) for r in results)
    return {"ok": all_ok, "results": results}


if __name__ == "__main__":
    mcp_server.run()
