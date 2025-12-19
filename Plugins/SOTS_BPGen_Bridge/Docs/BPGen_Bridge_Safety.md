# BPGen Bridge Safety (SPINE_L)

This document describes the safety and governance controls for the BPGen bridge.

## Local-only enforcement
- Default bind address is `127.0.0.1`.
- Non-loopback binds are blocked unless explicitly allowed:
  - Config: `[SOTS_BPGen_Bridge] bAllowNonLoopbackBind=true`
  - Env: `SOTS_BPGEN_ALLOW_NON_LOOPBACK=1`

## Auth token
- If `AuthToken` is set (or `SOTS_BPGEN_AUTH_TOKEN`), every request must include `params.auth_token`.
- Missing/mismatched tokens return `ERR_UNAUTHORIZED`.

## Allow/Deny action lists
- Config arrays:
  - `AllowedActions` (empty = allow all)
  - `DeniedActions` (always enforced)
- Denied actions return `ERR_ACTION_DENIED`.
- Missing from allowlist returns `ERR_ACTION_NOT_ALLOWED`.

## Dangerous operations + Safe Mode
Dangerous actions require `params.dangerous_ok=true`:
- `delete_node` / `delete_node_by_id`
- `delete_link`
- `replace_node`
- `batch` / `session_batch` when `atomic=true`
- `save_blueprint`

If safe mode is enabled, dangerous actions are blocked even with `dangerous_ok`:
- Error code: `ERR_SAFE_MODE_ACTIVE`

Enable/disable safe mode:
```json
{ "action": "set_safe_mode", "params": { "enabled": true } }
```

## Rate limits
- `MaxRequestsPerSecond` (default 60) and `MaxRequestsPerMinute` (default 0/off).
- Exceeding limits returns `ERR_RATE_LIMIT`.

## Audit logs
Every request is written to disk (auth token removed):
- Path: `Plugins/SOTS_BPGen_Bridge/Saved/BPGenAudit/YYYYMMDD/`
- File: `<request_id>_<action>.json`
- Includes request/response, timestamps, duration, and `change_summary` when available.

## Emergency stop
Action: `emergency_stop`
- Stops accepting new connections.
- Enables safe mode immediately.
- Returns `{ "stopped": true, "safe_mode": true }`.
