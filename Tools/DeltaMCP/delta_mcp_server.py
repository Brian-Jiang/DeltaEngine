import json
import socket
import pathlib
import sys

from mcp.server.fastmcp import FastMCP


def _find_repo_root() -> pathlib.Path:
    """Walk upward from this file until we find CMakePresets.json."""
    here = pathlib.Path(__file__).resolve()
    for parent in [here, *here.parents]:
        if (parent / "CMakePresets.json").exists():
            return parent
    raise RuntimeError(
        "Could not locate repo root (CMakePresets.json not found). "
        "Ensure delta_mcp_server.py is inside the repo tree."
    )


def _get_port() -> int:
    port_file = _find_repo_root() / "Intermediate" / "EditorState" / "DeltaEditor.port"
    if not port_file.exists():
        raise RuntimeError(
            f"Port file not found at {port_file}. "
            "Is DeltaEditor running?"
        )
    return int(port_file.read_text().strip())


mcp_server = FastMCP("DeltaEditor")

_sock: socket.socket | None = None
_buf: str = ""


def _send_command(payload: dict) -> dict:
    global _sock, _buf
    try:
        if _sock is None:
            _sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            _sock.settimeout(5.0)
            _sock.connect(("127.0.0.1", _get_port()))
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
def get_scene_state() -> dict:
    """
    Returns the full current scene as a JSON tree.
    Each entry in "objects" has: objectId, assetId, class, name,
    and a "components" array. Each component has: objectId, class,
    and a "properties" dict mapping property name to current value.
    Always call this first to discover objectIds before issuing any
    command that targets an existing object.
    """
    return _send_command({"type": "query", "query": "scene_state"})


@mcp_server.tool()
def get_class_schema(class_name: str) -> dict:
    """
    Returns all reflected property names and their types for a class.
    Call this before set_property to confirm the exact property name.
    Examples: "MeshRenderer", "PointLight", "SceneComponent", "Camera".
    The response contains a "properties" array of {name, type} objects.
    """
    return _send_command({"type": "query", "query": "class_schema",
                          "className": class_name})


@mcp_server.tool()
def create_game_object(name: str, parent_object_id: str = "") -> dict:
    """
    Creates a new empty GameObject in the active scene.
    Returns {"ok": true, "objectId": "<id>"} on success.
    Use the returned objectId to attach components or set properties.
    parent_object_id: objectId of the parent GameObject, or "" for root.
    """
    return _send_command({
        "type": "command",
        "command": "EditorCommand_CreateGameObject",
        "className": "GameObject",
    })


@mcp_server.tool()
def add_component(object_id: str, component_class: str) -> dict:
    """
    Adds a component to an existing GameObject.
    object_id: objectId of the target GameObject (from get_scene_state).
    component_class: reflected class name, e.g. "PointLight",
      "MeshRenderer", "Camera", "DirectionalLight", "SpotLight".
    Returns {"ok": true, "objectId": "<new component id>"}.
    """
    return _send_command({
        "type": "command",
        "command": "EditorCommand_CreateComponent",
        "gameObjectId": object_id,
        "className": component_class,
    })


@mcp_server.tool()
def set_property(object_id: str, property_name: str, value: object) -> dict:
    """
    Sets a reflected property on any DObject (GameObject or component).
    object_id: objectId from get_scene_state or a previous command response.
    property_name: exact name from get_class_schema (case-sensitive).
    value format by property type:
      float / int / bool  ->  JSON number or bool
      string              ->  JSON string
      Vec3                ->  [x, y, z]
      Quaternion          ->  [x, y, z, w]
      object reference    ->  {"objectId": "...", "assetId": "..."}
    Use get_class_schema() first to confirm the property name and type.
    """
    return _send_command({
        "type": "command",
        "command": "EditorCommand_SetProperty",
        "objectId": object_id,
        "propertyName": property_name,
        "valueAfter": value,
    })


@mcp_server.tool()
def rename_object(object_id: str, new_name: str) -> dict:
    """
    Renames a GameObject or component.
    object_id: objectId of the object to rename.
    new_name: the new display name.
    """
    return _send_command({
        "type": "command",
        "command": "EditorCommand_RenameObject",
        "targetObjectId": object_id,
        "newName": new_name,
    })


@mcp_server.tool()
def delete_game_object(object_id: str) -> dict:
    """
    Deletes a GameObject and all its components from the active scene.
    This action is undoable via Ctrl+Z in the editor.
    object_id: objectId of the GameObject to delete.
    """
    return _send_command({
        "type": "command",
        "command": "EditorCommand_DeleteGameObject",
        "gameObjectId": object_id,
    })


@mcp_server.tool()
def execute_batch(commands: list[dict]) -> dict:
    """
    Executes multiple commands as a single undoable batch.
    The entire batch appears as one undo entry in the editor (one Ctrl+Z
    reverses all commands in the batch).
    Each entry in `commands` is a plain dict with the same keys as the
    individual tool payloads -- include the "command" key in each entry.
    Example:
      [
        {"command": "EditorCommand_CreateGameObject", "name": "Sun"},
        {"command": "EditorCommand_CreateComponent",
         "objectId": "<use prior result>", "componentClass": "DirectionalLight"}
      ]
    Note: when object IDs from earlier commands in a batch are needed by
    later commands in the same batch, use separate sequential tool calls
    instead, then group unrelated mutations into a batch.
    """
    return _send_command({
        "type": "batch",
        "commands": commands,
    })


if __name__ == "__main__":
    mcp_server.run()
