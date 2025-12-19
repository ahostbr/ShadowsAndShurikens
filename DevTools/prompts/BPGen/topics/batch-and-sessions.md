# Batch and Sessions (SPINE_K)

Use batch when you need multiple BPGen actions to run as a single, ordered unit. Use sessions to keep caches warm across many calls.

## When to use batch
- Any workflow with 3+ steps (discover -> ensure -> apply -> verify -> compile/save).
- When you need per-step timing and a single summary report.
- When you want an atomic transaction wrapper (best-effort safety).

## Batch semantics (important)
- `atomic:true` wraps all steps in one `FScopedTransaction` + single-writer lock.
- `stop_on_error:true` stops after the first failed step (partial steps still returned).
- Atomic batches are best-effort; true rollback is not guaranteed without explicit undo calls.

## Session usage
Sessions let you reuse caches across multiple calls:
1) `begin_session` -> returns `session_id`
2) `session_batch` -> same as batch, but keeps session context
3) `end_session` -> clean up

Sessions expire after idle timeout (default 10 minutes).

## Recommended batch macro (golden workflow)
Use this structure for reliable graph edits:
1) `discover_nodes` (optional, cache warmup)
2) `ensure_function` / `ensure_variable` / `ensure_widget`
3) `apply_graph_spec` or `apply_graph_spec_to_target`
4) `list_nodes` or `describe_node` (verify)
5) `compile_blueprint`
6) `save_blueprint`

## Example batch request
```json
{
  "tool": "bpgen",
  "action": "batch",
  "request_id": "batch_01",
  "params": {
    "batch_id": "batch_01",
    "atomic": true,
    "stop_on_error": true,
    "commands": [
      { "action": "discover_nodes", "params": { "search_text": "Print String", "max_results": 50 } },
      { "action": "ensure_function", "params": { "blueprint_asset_path": "/Game/BP_Test", "function_name": "MyFunc" } },
      { "action": "apply_graph_spec", "params": { "blueprint_asset_path": "/Game/BP_Test", "function_name": "MyFunc", "graph_spec": { "nodes": [], "links": [] } } },
      { "action": "compile_blueprint", "params": { "blueprint_asset_path": "/Game/BP_Test" } },
      { "action": "save_blueprint", "params": { "blueprint_asset_path": "/Game/BP_Test" } }
    ]
  }
}
```

## Observability tips
- Use `server.request_ms`, `server.dispatch_ms`, and `server.serialize_ms` to spot slow calls.
- Use `get_recent_requests` to inspect the last N request summaries.
