# Buddy Worklog — 2025-12-25 11:42:00 — VibeUE Node Parity (Function Graphs)

## Goal
Implement the core VibeUE `manage_blueprint_node` actions on the unified SOTS MCP server by translating them into BPGen bridge calls (GraphSpec apply + graph edits), so upstream VibeUE-style prompts can actually build graphs.

## What changed
- Extended the VibeUE-compat tool `manage_blueprint_node` in `DevTools/python/sots_mcp_server/server.py`:
  - Added a more VibeUE-like signature (graph_scope, node_type, node_params, position/node_position, connect/disconnect arguments, property_name/property_value, extra).
  - Implemented function-graph actions backed by BPGen:
    - `create` → `bpgen_ensure_function` + `bpgen_apply` (GraphSpec Nodes-only)
    - `connect` / `connect_pins` → `bpgen_apply` (GraphSpec Links with NodeId references)
    - `configure` → `bpgen_apply` (GraphSpec ExtraData for literal defaults)
    - `move` → `bpgen_apply` (GraphSpec NodePosition)
    - `disconnect` / `disconnect_pins` → `bpgen_delete_link` (one-per-connection)
  - Uses BPGen `NodeId` as the stable identifier; for `create` we generate a GUID-like string and return it as `node_id`.
  - `discover/list/describe/delete` remain mapped to BPGen discover/list/describe/delete wrappers.
  - Explicitly returns "not implemented" for non-function `graph_scope` (event graphs would require `apply_graph_spec_to_target`).

## Files changed
- DevTools/python/sots_mcp_server/server.py

## Notes / Risks / Unknowns
- UNVERIFIED: BPGen bridge ping currently times out in `sots_smoketest_all` (so the new node actions cannot be executed until the UE-side bridge is reachable on the configured host/port).
- ASSUMPTION: BPGen `apply_graph_spec` supports linking existing nodes by supplying only `{Id, NodeId, allow_create:false}` without spawner details.
- The `bpgen_list` response parsing inside `create` is best-effort; the bridge response shape is `{"ok":..., "result":{...}}` and may evolve.
- Event graph parity is still TODO (needs `apply_graph_spec_to_target` routing + target selection contract).

## Verification
- VERIFIED (local): Python `py_compile` succeeds; VS Code diagnostics reports no errors.
- UNVERIFIED (runtime): Cannot validate `create/connect/configure/move/disconnect` against Unreal because BPGen ping timed out.

## Next steps
- Ryan: ensure BPGen bridge server is running and listening on the port configured in `.vscode/mcp.json` (`SOTS_BPGEN_PORT`). Re-run `sots_smoketest_all` until `bpgen_ping` succeeds.
- Buddy: once ping is green, run a minimal end-to-end test via MCP:
  - `manage_blueprint_function(action="create", ...)`
  - `manage_blueprint_node(action="create", ...)`
  - `manage_blueprint_node(action="configure", ...)`
  - `manage_blueprint_node(action="connect", ...)`
  - `manage_blueprint(action="compile", ...)`
