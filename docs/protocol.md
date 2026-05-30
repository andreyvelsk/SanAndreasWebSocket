# SanAndreasWebSocket — Protocol Reference

> Transport: **WebSocket** (text frames)  
> Message format: **JSON-RPC 2.0**  
> Default port: `8765`

---

## Table of Contents

1. [Connection](#1-connection)
2. [Message Structure](#2-message-structure)
3. [Methods — Client → Server](#3-methods--client--server)
   - [ping](#31-ping)
   - [query](#32-query)
   - [subscribe](#33-subscribe)
   - [unsubscribe](#34-unsubscribe)
   - [unsubscribe_all](#35-unsubscribe_all)
4. [Server Notifications](#4-server-notifications)
   - [data (push)](#41-data-push)
5. [Error Codes](#5-error-codes)
6. [Quick Examples](#6-quick-examples)

---

## 1. Connection

Connect to `ws://<host>:<port>` (e.g. `ws://127.0.0.1:8765`).  
All messages are UTF-8 JSON text frames. Binary frames are not supported.

---

## 2. Message Structure

### Request (client → server, expects a response)
```json
{
  "jsonrpc": "2.0",
  "method":  "<method_name>",
  "params":  { ... },
  "id":      "some_unique_id"
}
```
`params` may be omitted if the method has no parameters.

### Response — success
```json
{
  "jsonrpc": "2.0",
  "result":  <value>,
  "id":      "same_id_as_request"
}
```

### Response — error
```json
{
  "jsonrpc": "2.0",
  "error":   { "code": -32601, "message": "Method not found: foo" },
  "id":      "same_id_as_request"
}
```

### Notification (either side, no response expected)
```json
{
  "jsonrpc": "2.0",
  "method":  "<method_name>",
  "params":  { ... }
}
```
Omitting `"id"` makes the message a notification.

---

## 3. Methods — Client → Server

### 3.1 ping

Returns immediately. Useful for connection health checks.

**Request**
```json
{"jsonrpc": "2.0", "method": "ping", "id": "p1"}
```

**Response**
```json
{"jsonrpc": "2.0", "result": null, "id": "p1"}
```

---

### 3.2 query

Read the current value of one or more game fields in a single round-trip.  
The server reads the values on the **game thread** and replies asynchronously.

**Request**
```json
{
  "jsonrpc": "2.0",
  "method":  "query",
  "params":  { "fields": ["health", "position", "money"] },
  "id":      "q1"
}
```

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `fields`  | `string[]` | yes | Names of fields to read. See [fields.md](fields.md) |

**Response**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "ts": 1234567890,
    "fields": {
      "health":   100.0,
      "position": { "x": 1234.5, "y": -1000.2, "z": 11.0 },
      "money":    5000
    }
  },
  "id": "q1"
}
```

`ts` — server timestamp (`GetTickCount()`, milliseconds since system boot).

**Errors**

| Code | When |
|------|------|
| `-32602` | `fields` parameter missing or not an array |
| `-32002` | One of the requested field names is not registered |

---

### 3.3 subscribe

Subscribe to push updates for one or more fields.  
The server will send a `data` notification whenever a subscribed field changes,
checked at the configured `interval_ms` interval.

Calling `subscribe` again with new fields **adds** them to the existing subscription
and optionally updates the interval.

**Request**
```json
{
  "jsonrpc": "2.0",
  "method":  "subscribe",
  "params":  {
    "fields":      ["position", "health"],
    "interval_ms": 200
  },
  "id": "s1"
}
```

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `fields`      | `string[]` | yes | — | Fields to subscribe to |
| `interval_ms` | `integer`  | no  | `500` | Check interval in milliseconds. Minimum: `50` |

**Response**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "subscribed":  ["position", "health"],
    "interval_ms": 200
  },
  "id": "s1"
}
```

`subscribed` — the full set of currently active subscriptions for this connection.

**Notes**
- A value is only pushed when it **changes** relative to its last sent value.
- `position` as a composite field is compared as a whole object.
- If the player is not available (`FindPlayerPed()` returns null), the field value
  will be `null` — a change from a valid value to `null` **is** reported.

**Errors**

| Code | When |
|------|------|
| `-32602` | `fields` missing or not an array |
| `-32002` | Unknown field name |

---

### 3.4 unsubscribe

Remove specific fields from the subscription. The push timer continues running
for the remaining subscribed fields.

**Request**
```json
{
  "jsonrpc": "2.0",
  "method":  "unsubscribe",
  "params":  { "fields": ["health"] },
  "id":      "s2"
}
```

**Response**
```json
{"jsonrpc": "2.0", "result": null, "id": "s2"}
```

---

### 3.5 unsubscribe_all

Cancel all subscriptions and stop the push timer.

**Request**
```json
{"jsonrpc": "2.0", "method": "unsubscribe_all", "id": "s3"}
```

**Response**
```json
{"jsonrpc": "2.0", "result": null, "id": "s3"}
```

---

## 4. Server Notifications

Server-side notifications follow the JSON-RPC 2.0 notification format (no `"id"` field).
The client must not reply to them.

### 4.1 data (push)

Sent when one or more subscribed fields have changed since the last push.
Only changed fields are included.

```json
{
  "jsonrpc": "2.0",
  "method":  "data",
  "params":  {
    "ts":     1234567891,
    "fields": {
      "health":   75.0,
      "position": { "x": 1240.1, "y": -998.7, "z": 11.0 }
    }
  }
}
```

---

## 5. Error Codes

### Standard JSON-RPC 2.0

| Code | Name | Description |
|------|------|-------------|
| `-32700` | `PARSE_ERROR` | Received text is not valid JSON |
| `-32600` | `INVALID_REQUEST` | JSON object is not a valid JSON-RPC 2.0 request |
| `-32601` | `METHOD_NOT_FOUND` | Requested method does not exist |
| `-32602` | `INVALID_PARAMS` | Parameters are missing or have wrong type |
| `-32603` | `INTERNAL_ERROR` | Internal JSON-RPC error |

### Server-Defined (range `-32000` … `-32099`)

| Code | Name | Description |
|------|------|-------------|
| `-32000` | `GAME_NOT_READY` | Game not yet initialised (no active player session) |
| `-32001` | `PLAYER_NOT_FOUND` | `FindPlayerPed()` returned `nullptr` |
| `-32002` | `UNKNOWN_FIELD` | Requested field name is not in the registry |

---

## 6. Quick Examples

### Read player position (one-shot)

```json
→ {"jsonrpc":"2.0","method":"query","params":{"fields":["position"]},"id":"1"}
← {"jsonrpc":"2.0","result":{"ts":3600000,"fields":{"position":{"x":2496.0,"y":-1667.6,"z":13.4}}},"id":"1"}
```

### Subscribe to position updates every 100 ms

```json
→ {"jsonrpc":"2.0","method":"subscribe","params":{"fields":["position"],"interval_ms":100},"id":"2"}
← {"jsonrpc":"2.0","result":{"subscribed":["position"],"interval_ms":100},"id":"2"}

// whenever position changes:
← {"jsonrpc":"2.0","method":"data","params":{"ts":3600100,"fields":{"position":{"x":2498.3,"y":-1669.1,"z":13.4}}}}
← {"jsonrpc":"2.0","method":"data","params":{"ts":3600200,"fields":{"position":{"x":2501.7,"y":-1670.8,"z":13.4}}}}
```

### Subscribe to health + position, then stop position updates

```json
→ {"jsonrpc":"2.0","method":"subscribe","params":{"fields":["health","position"],"interval_ms":500},"id":"3"}
← {"jsonrpc":"2.0","result":{"subscribed":["health","position"],"interval_ms":500},"id":"3"}

→ {"jsonrpc":"2.0","method":"unsubscribe","params":{"fields":["position"]},"id":"4"}
← {"jsonrpc":"2.0","result":null,"id":"4"}
```

### Query multiple fields at once

```json
→ {
    "jsonrpc":"2.0","method":"query",
    "params":{"fields":["health","armour","money","wanted","position","in_vehicle","game_time"]},
    "id":"snapshot"
  }
← {
    "jsonrpc":"2.0",
    "result":{
      "ts":3601000,
      "fields":{
        "health":100.0, "armour":50.0, "money":12500, "wanted":0,
        "position":{"x":2496.0,"y":-1667.6,"z":13.4},
        "in_vehicle":false,
        "game_time":{"hour":14,"minute":32}
      }
    },
    "id":"snapshot"
  }
```

### Error: unknown field

```json
→ {"jsonrpc":"2.0","method":"query","params":{"fields":["nonexistent"]},"id":"err1"}
← {"jsonrpc":"2.0","error":{"code":-32002,"message":"Unknown field: nonexistent"},"id":"err1"}
```
