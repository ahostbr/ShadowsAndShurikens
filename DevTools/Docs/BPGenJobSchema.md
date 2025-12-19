# BPGen Job Schema (DevTools Inbox)

## Locations
- Inbox: `DevTools/Inbox/BPGenJobs/`
- Reports: `DevTools/Reports/BPGen/`

## Envelope
Single job file is JSON:
```
{
  "job_id": "bpgen_<id>",
  "action": "apply_graph_spec",   // or use batch array instead
  "params": { ... },               // matches BPGen action params
  "batch": [                       // optional batch instead of action/params
    { "action": "canonicalize_graph_spec", "params": { ... } },
    { "action": "apply_graph_spec", "params": { ... } }
  ],
  "allow_apply": false,            // default false; set true to allow mutations
  "request_id": "optional"       // echoed into reports
}
```

## Processing expectations
- Tooling should set `allow_apply=true` only when `SOTS_ALLOW_APPLY=1` is also present in environment.
- Batch items execute in order; stop on failure.
- Reports mirror the job id under `DevTools/Reports/BPGen/<job_id>.json` and include action results and errors.

## Action names (must match BPGen API)
- `apply_graph_spec`
- `canonicalize_graph_spec`
- `apply_function_skeleton`
- `delete_node`
- `delete_link`
- `replace_node`
- `create_struct_asset`
- `create_enum_asset`
- (Optional bridge helpers) `compile_blueprint`, `save_blueprint` when routed through bridge.
