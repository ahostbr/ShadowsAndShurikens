# BPGen Result Schemas (SPINE_P)

All BPGen API/bridge responses share the envelope:

```json
{
  "action": "apply_graph_spec",
  "request_id": "optional",
  "ok": true,
  "error": "",
  "errors": ["..."],
  "warnings": ["..."],
  "result": { /* action-specific */ }
}
```

## Common Result Shapes

### apply_graph_spec / apply_function_skeleton / delete_link / delete_node / replace_node
```json
{
  "blueprint_asset_path": "/Game/Path.BP.Path",
  "function_name": "Func",
  "bSuccess": true,
  "created_node_ids": ["NodeA"],
  "updated_node_ids": ["NodeB"],
  "deleted_node_ids": [],
  "warnings": [""],
  "errors": [""],
  "change_summary": {}
}
```

### canonicalize_graph_spec / canonicalize_spec
```json
{
  "bSuccess": true,
  "canonical_spec": { /* GraphSpec after normalization */ },
  "warnings": [""],
  "errors": [""],
  "migrations_applied": ["1->1"],
  "spec_version": 1
}
```

### ensure_function / ensure_variable / ensure_widget
```json
{
  "bSuccess": true,
  "created": true,
  "updated": false,
  "warnings": [],
  "errors": []
}
```

### debug_annotate / clear_annotations / focus_node
```json
{ "ok": true }
```

### server_info
Mirrors `GetServerInfoForUI` fields: bind_address, port, running, safe_mode, allow_non_loopback_bind, max_request_bytes, max_requests_per_second, max_requests_per_minute, uptime_seconds, started_utc, last_start_error, last_start_error_code, total_requests, features { ... }.

## Deprecation/Migrations
- Fields are additive; do not remove/rename existing keys.
- Spec changes tracked by `spec_version`; use canonicalize before apply when versions differ.
- Clients should ignore unknown fields to stay forward-compatible.
