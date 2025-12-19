# BPGen Action Reference (SPINE_P)

## BPGen API (One True Entry)
Stable action names (editor-only). Inputs are JSON objects; responses use the standard envelope (see ResultSchemas).

| Action | Purpose | Notes |
| --- | --- | --- |
| `apply_graph_spec` | Apply a GraphSpec to a target function/graph. | Mutations gated by `SOTS_ALLOW_APPLY=1`. |
| `canonicalize_graph_spec` | Normalize/patch a GraphSpec. | Returns canonicalized spec + warnings. |
| `apply_function_skeleton` | Create/ensure a function signature. | Editor-only; respects apply gate. |
| `delete_node` | Remove a node by id. | Dangerous; use `dangerous_ok` via bridge. |
| `delete_link` | Remove a link. | Same gating as delete_node. |
| `replace_node` | Replace a node preserving NodeId. | Best-effort. |
| `create_struct_asset` | Create a data struct asset. | Editor-only asset creation. |
| `create_enum_asset` | Create an enum asset. | Editor-only asset creation. |
| `debug_annotate` | Add `[BPGEN]` annotation bubble to NodeIds. | Editor visual debug. |
| `clear_annotations` | Remove `[BPGEN]` annotations. | Editor visual debug. |
| `focus_node` | Focus the Blueprint editor on a NodeId. | Best-effort UI jump. |

## Bridge Actions (TCP NDJSON)
Mirror the API above plus bridge operations. Defaults: bind 127.0.0.1:55557, protocol_version 1.0.

| Action | Purpose | Notes |
| --- | --- | --- |
| `ping` | Health probe; returns protocol_version. | Use to validate connectivity. |
| `server_info` | Bind, uptime, limits, features, safe_mode. | Matches Control Center status view. |
| `get_spec_schema` | Returns spec schema/version. | spec_version currently 1. |
| `canonicalize_spec` | Same intent as `canonicalize_graph_spec`. | Accepts `graph_spec` + optional `options`. |
| `apply_graph_spec` | GraphSpec apply via bridge. | Honours `SOTS_ALLOW_APPLY` in upstream. |
| `ensure_function` / `ensure_variable` / `ensure_widget` | Ensure targets exist. | Feature-flagged; see server_info.features. |
| `list_nodes` / `describe_node` | Introspection helpers. | Editor-only. |
| `delete_node` / `delete_link` / `replace_node` | Graph edits. | Require `dangerous_ok=true` when safe_mode enabled. |
| `batch` / `session_batch` | Multiple actions in one request. | Session aware; subject to limits. |
| `get_recent_requests` | Returns recent request ring buffer. | UI uses `GetRecentRequestsForUI` internally. |
| `health` | Basic health payload. | Mirrors server_info with ok/limits. |
| `set_safe_mode` / `emergency_stop` | Safety controls. | Default safe_mode=true recommended for new installs. |
| `shutdown` | Stop the bridge server. | Editor-only; local use.

## Feature Flags
`server_info.features` and bridge responses include stable flags: targets, ensure_function, ensure_variable, umg, describe_node_links, error_codes, graph_edits, auto_fix, recipes, batch, sessions, cache_controls, limits, recent_requests, server_info, spec_schema, canonicalize_spec, health, safety, audit, dry_run, auth_token, rate_limit.

## Deprecation Policy
- Never remove/rename action names; only add aliases.
- Keep old response fields for at least one minor version; prefer additive changes.
- If an action is superseded, document the replacement and keep the old path as a shim until next major.
