# Buddy Worklog — Event Graph Node Parity (BPGen)

goal
- Tackle the next parity slice by enabling VibeUE-style node creation/wiring in Event Graph scope using BPGen `apply_graph_spec_to_target`.

what changed
- Added a BPGen MCP tool wrapper: `bpgen_apply_to_target` → bridge action `apply_graph_spec_to_target`.
- Extended VibeUE-compat `manage_blueprint_node` to support `graph_scope="event"` for:
  - `create`, `connect` / `connect_pins`, `configure`, `move`
  - These actions now build a GraphSpec with `target_type=EventGraph` and call `bpgen_apply_to_target`.
- Left deletion/disconnect operations function-only because BPGen graph edit actions require `function_name` and currently do not accept `target` descriptors.
- Updated parity matrix documentation accordingly.

files changed
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

notes + risks/unknowns
- UNVERIFIED: no Unreal/bridge runtime validation performed.
- Event graph name resolution: BPGen GraphResolver defaults EventGraph name to "EventGraph" when `target.name` is empty; mapping relies on this behavior.
- `manage_blueprint_node` event scope still lacks `list/describe/delete/disconnect` parity due to missing target-based introspection/edit actions.

verification status
- UNVERIFIED (syntax-only check via `python -m py_compile`)

follow-ups / next steps
- Consider adding target-based graph edit actions to the bridge (delete_node/link with target) if event-graph delete/disconnect parity is required.
- Ryan: validate event-graph node creation and wiring against a live UE session.
