# BPGen Bridge Protocol (SPINE_F)

Local-only TCP NDJSON bridge for BPGen.

## Defaults
- Address: 127.0.0.1
- Port: 55557
- Framing: newline-delimited JSON (one request per connection, one response, then close)
- Timeouts: socket read ~2-5s idle, game thread dispatch 30s

## Request
```json
{
  "tool": "bpgen",
  "action": "ping" | "discover_nodes" | "apply_graph_spec" | "apply_graph_spec_to_target" | "ensure_function" | "ensure_variable" | "ensure_widget" | "set_widget_properties" | "ensure_binding" | "list_nodes" | "describe_node" | "compile_blueprint" | "save_blueprint" | "refresh_nodes" | "shutdown",
  "request_id": "any string",
  "params": { "...": "action-specific" }
}
```

## Response
```json
{
  "ok": true,
  "request_id": "...",
  "action": "...",
  "result": { "...": "action-specific" },
  "errors": ["..."],
  "warnings": ["..."],
  "server.plugin": "SOTS_BPGen_Bridge",
  "server.version": "0.1",
  "server.protocol_version": "1.0",
  "server.features": { "targets": true, "ensure_function": true, "ensure_variable": true, "umg": true, "describe_node_links": true, "error_codes": true, "graph_edits": true, "auto_fix": true, "recipes": true, "dry_run": true, "auth_token": false, "rate_limit": true },
  "server.port": 55557,
  "error_code": "ERR_PROTOCOL_MISMATCH" // optional (also ERR_UNAUTHORIZED, ERR_RATE_LIMIT, ERR_REQUEST_TOO_LARGE)
}
```

### ProtocolVersion & Features
- Clients may send `params.client_protocol_version` (e.g. `"1.0"`). If the major version mismatches the server, the bridge responds `ok:false` with `error_code:"ERR_PROTOCOL_MISMATCH"`.
- Responses always include `server.protocol_version` and `server.features` to advertise supported surfaces.
- If the bridge is configured with an auth token, every request must include `params.auth_token` or the response will return `ERR_UNAUTHORIZED`.
- Requests that exceed the configured size cap return `ERR_REQUEST_TOO_LARGE`. Rate limiting (if enabled) returns `ERR_RATE_LIMIT`.
- Bridge config keys (DefaultEngine.ini `[SOTS_BPGen_Bridge]` or environment): `AuthToken` / `SOTS_BPGEN_AUTH_TOKEN`, `MaxRequestsPerSecond` / `SOTS_BPGEN_MAX_RPS`.

## Supported actions
- `ping`: returns `pong:true`, UTC `time`, `version`.
- `discover_nodes`: params `blueprint_asset_path` (optional), `search_text`, `max_results` (int), `include_pins` (bool). Returns `FSOTS_BPGenNodeDiscoveryResult`.
- `apply_graph_spec`: params include either `blueprint_asset_path`, `function_name`, `graph_spec` or nested `function_def` and `graph_spec` objects matching BPGen JSON. GraphSpec supports optional `auto_fix`, `auto_fix_max_steps`, `auto_fix_insert_conversions`. Routes to `USOTS_BPGenBuilder::ApplyGraphSpecToFunction`.
- `apply_graph_spec_to_target`: params `graph_spec` (with optional `target` block). Routes to `USOTS_BPGenBuilder::ApplyGraphSpecToTarget` for non-function graphs.
- `ensure_function`: params `blueprint_asset_path`, `function_name`, optional `signature` (FSOTS_BPGenFunctionSignature JSON), `create_if_missing` (bool, default true), `update_if_exists` (bool, default true). Returns `FSOTS_BPGenEnsureResult`.
- `ensure_variable`: params `blueprint_asset_path`, `variable_name`, `pin_type` (FSOTS_BPGenPin JSON), optional `default_value` (string), `instance_editable` (bool, default true), `expose_on_spawn` (bool, default false), `create_if_missing` (bool), `update_if_exists` (bool). Returns `FSOTS_BPGenEnsureResult`.
- `ensure_widget`: params follow `FSOTS_BPGenWidgetSpec` (blueprint_asset_path, widget_class_path, widget_name, parent_name, insert_index, is_variable, create_if_missing, update_if_exists, reparent_if_mismatch, properties map, slot_properties map). Returns `FSOTS_BPGenWidgetEnsureResult`.
- `set_widget_properties`: params follow `FSOTS_BPGenWidgetPropertyRequest` (blueprint_asset_path, widget_name, properties map, fail_if_missing). Returns `FSOTS_BPGenWidgetPropertyResult`.
- `ensure_binding`: params follow `FSOTS_BPGenBindingRequest` (blueprint_asset_path, widget_name, property_name, function_name, signature, graph_spec, apply_graph, create_binding, update_binding, create_function, update_function). Returns `FSOTS_BPGenBindingEnsureResult`.
- `list_nodes`: params `blueprint_asset_path`, `function_name`, `include_pins` (bool). Returns `FSOTS_BPGenNodeListResult`.
- `describe_node`: params `blueprint_asset_path`, `function_name`, `node_id`, `include_pins` (bool). Returns `FSOTS_BPGenNodeDescribeResult`.
- `compile_blueprint`: params `blueprint_asset_path`. Returns `FSOTS_BPGenMaintenanceResult`.
- `save_blueprint`: params `blueprint_asset_path`. Returns `FSOTS_BPGenMaintenanceResult`.
- `refresh_nodes`: params `blueprint_asset_path`, `function_name`, `include_pins` (bool). Returns `FSOTS_BPGenMaintenanceResult`.
- `delete_node` | `delete_node_by_id`: params `FSOTS_BPGenDeleteNodeRequest` (blueprint_asset_path, function_name, node_id, compile). Returns `FSOTS_BPGenGraphEditResult`.
- `delete_link`: params `FSOTS_BPGenDeleteLinkRequest` (blueprint_asset_path, function_name, from_node_id, from_pin, to_node_id, to_pin, compile). Returns `FSOTS_BPGenGraphEditResult`.
- `replace_node`: params `FSOTS_BPGenReplaceNodeRequest` (blueprint_asset_path, function_name, existing_node_id, new_node spec, pin_remap map, compile). Returns `FSOTS_BPGenGraphEditResult`.
- `recipe_print_string`: params `blueprint_asset_path` or `target`, `message`, `node_id_prefix`. Returns `expanded_graph_spec` + `apply_result`.
- `recipe_branch_on_bool`: params `blueprint_asset_path` or `target`, `condition`, `node_id_prefix`. Returns `expanded_graph_spec` + `apply_result`.
- `recipe_set_variable`: params `blueprint_asset_path` or `target`, `variable_name`, `pin_type`, optional `value`/`default_value`, `node_id_prefix`. Returns `expanded_graph_spec` + `apply_result` + `ensure_result`.
- `recipe_widget_bind_text`: params `blueprint_asset_path`, `widget_name`, `function_name`, optional `property_name`, `message`, `node_id_prefix`. Returns `expanded_graph_spec` + `apply_result` + `binding_result`.
- `shutdown`: responds then stops the server.

## Notes
- All BPGen mutations are dispatched onto the Game Thread before responding.
- Mutating actions accept `dry_run:true` to skip changes and return a preview result with `dry_run` metadata.
- If `server.features.auth_token` is true, pass `params.auth_token` on every request (DevTools can inject via `SOTS_BPGEN_AUTH_TOKEN`).
- One connection at a time; sequential handling.
- Protocol intentionally small to match MCP-style tooling.
