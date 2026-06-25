# DeltaMCP

FastMCP server that exposes DeltaEditor's MCP command system to AI assistants.

## Requirements

- DeltaEditor must be running before starting the server.
- Uses the bundled Python interpreter at `Tools/Python/python.exe`.
- The `mcp` package must be installed in that environment (part of the DeltaEngine toolchain setup).

## Running

```
Tools/Python/python.exe Tools/DeltaMCP/delta_mcp_server.py
```

The server reads the editor TCP port from `Intermediate/EditorState/DeltaEditor.port`.

## Wire Protocol

See [PROTOCOL.md](PROTOCOL.md) for the authoritative wire specification.

- **Queries** are synchronous: one request, one response.
- **Commands** use a two-phase protocol over TCP. The Python client may return `execution_pending: true` when the editor is slow to execute; follow up with scene queries rather than retrying the same command.

## MCP Tools

| Tool | Description |
|---|---|
| `list_operations` | Index of all systems, queries, and commands |
| `describe_operations` | Full parameter schemas for specific queries/commands |
| `execute_batch` | Run one or more queries and/or commands sequentially |

## Agent Workflow

1. Call `list_operations` to discover available systems, queries, and commands.
2. Call `describe_operations` for the operations you need.
3. Query scene state (e.g. `scene/game_objects`) to obtain `objectId` values for existing objects.
4. Call `execute_batch` with your queries and/or commands.

Each command in a batch is its own undo entry. Commands that need a result (creates, deletes, set-property, etc.) wait for main-thread execution; others return after the immediate accept envelope.

## Schemas

Parameter schemas live in `Tools/DeltaMCP/Schemas/*.json` (one file per MCP system).

## Tests

```
Tools\Scripts\test-delta-mcp.bat --automatic
```
