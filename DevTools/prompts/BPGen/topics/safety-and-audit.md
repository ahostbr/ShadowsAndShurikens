# Safety and Audit (SPINE_L)

This topic covers guardrails for BPGen: auth token, safe mode, dangerous operations, and audit logs.

## Auth token
- If the bridge is configured with `AuthToken`, every request must include `params.auth_token`.
- DevTools can inject automatically via `SOTS_BPGEN_AUTH_TOKEN`.

## Dangerous operations
Dangerous actions require explicit opt-in:
- delete_node / delete_node_by_id
- delete_link
- replace_node
- batch / session_batch (when `atomic=true`)
- save_blueprint

Add to params:
```json
{ "dangerous_ok": true }
```

If you omit `dangerous_ok`, the bridge returns:
- `ERR_DANGEROUS_OP_REQUIRES_OPT_IN`

## Safe mode
- Safe mode blocks dangerous operations even with `dangerous_ok`.
- Toggle via `set_safe_mode`:
```json
{ "action": "set_safe_mode", "params": { "enabled": true } }
```

If safe mode is active, dangerous ops return:
- `ERR_SAFE_MODE_ACTIVE`

## Audit logs
Every request is audited (auth token removed):
- Location: `Plugins/SOTS_BPGen_Bridge/Saved/BPGenAudit/YYYYMMDD/`
- Filename: `<request_id>_<action>.json`
- Includes request, response, timing, and `change_summary` if present.

## Tips
- Use `server_info` or `health` to check safe mode and rate limits.
- Always attach audit logs when reporting failures or regressions.
