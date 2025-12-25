Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1230 | Plugin: sots_mcp_server | Pass/Topic: EventGraphNodeParity | Owner: Buddy
Scope: Enable manage_blueprint_node event-graph create/connect/configure/move via BPGen apply_graph_spec_to_target

DONE
- Added `bpgen_apply_to_target` MCP tool wrapper (maps to bridge `apply_graph_spec_to_target`).
- Updated `manage_blueprint_node` to support graph_scope="event" for create/connect/connect_pins/configure/move by emitting a GraphSpec with:
  - target.blueprint_asset_path = blueprint_name
  - target.target_type = EventGraph
  - target.name = "" (GraphResolver defaults to EventGraph)
- Updated parity matrix notes for node tool family.

VERIFIED
- UNVERIFIED (no UE/bridge run). Syntax-only check performed.

UNVERIFIED / ASSUMPTIONS
- Assumes bridge action `apply_graph_spec_to_target` accepts snake_case keys (`target.blueprint_asset_path`, `target.target_type`, `target.name`) as documented.
- Assumes EventGraph exists or is creatable when `create_if_missing=true` (default).

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

NEXT (Ryan)
- With UE running and BPGen bridge reachable, call manage_blueprint_node.create (graph_scope="event") on a known BP; confirm node appears in EventGraph.
- Call manage_blueprint_node.connect_pins (graph_scope="event") to wire exec flow; confirm links.
- Call manage_blueprint_node.configure/move (graph_scope="event") and confirm pin default + layout changes.

ROLLBACK
- Revert the two touched files above (or git revert the change).
