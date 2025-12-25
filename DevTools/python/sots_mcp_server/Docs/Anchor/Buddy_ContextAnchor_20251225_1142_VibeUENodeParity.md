Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1142 | Plugin: sots_mcp_server | Pass/Topic: VIBEUE_NODE_PARITY | Owner: Buddy
Scope: Implement core VibeUE `manage_blueprint_node` actions for function graphs using BPGen apply_graph_spec + delete_link.

DONE
- Extended `manage_blueprint_node` (VibeUE-compat) to support function graph mutations:
  - `create` → `bpgen_ensure_function` + `bpgen_apply` (Nodes-only GraphSpec)
  - `connect` / `connect_pins` → `bpgen_apply` (Links GraphSpec referencing existing nodes by NodeId)
  - `configure` → `bpgen_apply` (ExtraData literal pin defaults)
  - `move` → `bpgen_apply` (NodePosition)
  - `disconnect` / `disconnect_pins` → `bpgen_delete_link`
- Added helper utilities for GUID generation and BPGen literal/Vec2 formatting.
- Updated parity tracking doc to mark node actions implemented for function graphs.

VERIFIED
- Python `py_compile` for `server.py` passes.

UNVERIFIED / ASSUMPTIONS
- BPGen bridge was unreachable during smoke test (`bpgen_ping` timed out), so node actions were not executed against Unreal.
- Event graph parity is not implemented (would require `apply_graph_spec_to_target`).

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- DevTools/python/sots_mcp_server/Docs/Worklogs/BuddyWorklog_20251225_114200_VibeUENodeParity.md
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

NEXT (Ryan)
- Ensure the UE BPGen bridge is listening on the configured host/port (`SOTS_BPGEN_HOST`, `SOTS_BPGEN_PORT`) and re-run `sots_smoketest_all` until `bpgen_ping` succeeds.
- Then validate an end-to-end graph op sequence via MCP:
  - create function, create node, configure node pin default, connect pins, compile/save.

ROLLBACK
- Revert `DevTools/python/sots_mcp_server/server.py` and revert the parity matrix edit if undesired.
