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
| `request_id` | no | Correlation id assigned by the Python client; the editor echoes it on the response line. If omitted, no id is echoed. |

Example query:

```json
{"type":"query","system":"meta","query":"list_operations","params":{}}
```

Example command:

```json
{"type":"command","system":"scene","command":"CreateGameObject","params":{"name":"Cube"},"request_id":"42"}
```

## Responses — Single-Phase, Synchronous

Both queries and commands return exactly **one** response line per request. The
editor's socket thread hands each request to the main thread, which executes it
(query or command) between frames and replies once. There is no `phase` field and
no separate accept/result handshake.

Query:

```json
{"ok":true,"systems":{...}}
```

Command:

```json
{
  "ok": true,
  "commandType": "EditorCommand_CreateGameObject",
  "objectId": "...",
  "request_id": "42"
}
```

Error:

```json
{"ok":false,"error":"..."}
```

`request_id` is echoed on the response when the client supplied one. Pre-parse
failures (empty body, malformed JSON) return bare `{ok:false, error}`.

`CreateGameObject` with a custom name runs create then rename on the main thread and
returns one response. If rename fails after create succeeds, the result includes
`objectId` plus an `error` describing the rename failure.

## Threading Model

- The MCP socket runs on its own thread purely as transport. It never touches the
  engine object graph.
- Each request is queued to the editor main thread and executed once per frame
  (before tick/GC), so queries observe a consistent snapshot and commands run with
  full engine access. The socket thread blocks on the result, then writes it back.

## Python Client Behavior

Timeout constant (see `delta_mcp_server.py`):

| Operation | Timeout |
|-----------|---------|
| Command result | 30.0 seconds |
| Connect / query read | 5.0 seconds |

1. Assign a monotonic `request_id` on every outbound command; C++ echoes it.
2. Read the single response line with the matching `request_id`.
3. On timeout → return a structured error; do not close the socket. Re-query scene
   state rather than blindly retrying a mutating command.
4. Stray lines with a non-matching `request_id` (e.g. a late reply from a previously
   timed-out request) are skipped.

Wire fields stripped from responses shown to callers: `request_id` (and any legacy
`phase`, `queued`, `expects_result`).
