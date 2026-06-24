# DeltaMCP Wire Protocol

Newline-delimited JSON over TCP (default port 57340).

## Inbound Envelope

Every request is a single JSON object terminated by `\n`.

| Field | Required | Description |
|-------|----------|-------------|
| `type` | no (default `"query"`) | `"query"` or `"command"` |
| `system` | yes | Target MCP system name |
| `query` | queries only | Query name |
| `command` | commands only | Command name |
| `params` | no (default `{}`) | Operation parameters |
| `request_id` | no | Correlation id assigned by the Python client; C++ echoes it on every command response line. If omitted, the editor generates a server-side UUID. |

Example query:

```json
{"type":"query","system":"meta","query":"list_operations","params":{}}
```

Example command:

```json
{"type":"command","system":"scene","command":"CreateGameObject","params":{"name":"Cube"},"request_id":"42"}
```

## Query Response

Queries are synchronous: one request → one response. No `phase` field.

```json
{"ok":true,"systems":{...}}
```

Error:

```json
{"ok":false,"error":"..."}
```

Pre-parse failures (empty body, malformed JSON) return bare `{ok:false, error}` with no `phase` because the request type is unknown.

## Command Responses — Two-Phase Protocol

Commands use two response lines when execution produces a meaningful result after main-thread drain.

### Phase 1 — Accept (immediate, IO thread)

Sent immediately after validation and enqueue. Always includes `phase` and `request_id`.

```json
{
  "phase": "accept",
  "request_id": "42",
  "ok": true,
  "expects_result": true,
  "command": "CreateGameObject",
  "queued": true
}
```

Handler payload fields (`count`, `can_undo`, `error`, etc.) are merged into the accept envelope.

| Field | Description |
|-------|-------------|
| `phase` | Always `"accept"` |
| `request_id` | Echo of inbound id (or server-generated) |
| `ok` | Whether the command was accepted |
| `expects_result` | Whether the client should wait for a phase-2 result line |
| `command` | MCP command name (when applicable) |
| `queued` | Present when work was enqueued for main-thread execution |

When `ok:false` or `expects_result:false`, the accept line is the final response for that command.

### Phase 2 — Result (main thread, after DrainCommandQueue)

**Not yet emitted by C++ (future phase).** Documented shape:

```json
{
  "phase": "result",
  "request_id": "42",
  "ok": true,
  "commandType": "EditorCommand_CreateGameObject",
  "objectId": "..."
}
```

Until phase-2 tagging is implemented, drain responses are sent as untagged flat JSON on the same socket connection.

## expects_result Rules

| expects_result | When |
|----------------|------|
| `false` | Validation failures (`ok:false`) |
| `false` | `SaveProject` |
| `false` | Animated transforms/lights (`duration_seconds > 0`) |
| `false` | Sync commands: `SelectObject`, `Undo`, `Redo`, `SetViewportCamera` |
| `false` | Commands with no useful drain payload: `LoadScene` |
| `true` | Creates, deletes, set-property, rename, reparent, reimport, metadata commands that return meaningful drain data (especially `objectId`) |

## Python Client Behavior (future)

1. Read phase-1 accept immediately.
2. If `ok:false` or `expects_result:false` → return stripped accept to caller.
3. Else wait up to N seconds for phase-2 with matching `request_id`.
4. On timeout → return accept + `execution_pending:true`; do not close the socket.
5. Buffer or discard orphan phase-2 lines so later commands are not corrupted.

Wire fields stripped from normal success responses: `phase`, `request_id`, `queued`, `expects_result`.
