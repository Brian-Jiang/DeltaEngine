# DeltaMCP

MCP server that exposes DeltaEditor's command system to AI assistants.

## Requirements
Uses the bundled Python interpreter at `Tools/Python/python.exe`.
The `mcp` package must be installed in that environment (already done as
part of the DeltaEngine toolchain setup).

## Running
```
Tools/Python/python.exe Tools/DeltaMCP/delta_mcp_server.py
```
DeltaEditor must be running before starting the server.
The server reads the editor's port from:
`Intermediate/EditorState/DeltaEditor.port`

## Tools exposed

| Tool | Description |
|---|---|
| get_scene_state | Full scene snapshot with objectIds |
| get_class_schema | Property names + types for any reflected class |
| create_game_object | Create a new GameObject |
| add_component | Add a component to a GameObject |
| set_property | Set any reflected property |
| rename_object | Rename a GameObject or component |
| delete_game_object | Delete a GameObject |
| execute_batch | Run multiple commands as one undoable unit |
