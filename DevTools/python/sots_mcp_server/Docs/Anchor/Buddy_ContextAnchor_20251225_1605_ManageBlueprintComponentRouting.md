Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1605 | Plugin: sots_mcp_server | Pass/Topic: manage_blueprint_component routing | Owner: Buddy
Scope: Unified MCP implementation of VibeUE-compat component tool + parity doc update

DONE
- Implemented `manage_blueprint_component` in `DevTools/python/sots_mcp_server/server.py`.
- Routed actions to BPGen bridge component ops (`bp_component_*`) and added Python-side `compare_properties`.
- Updated parity status for tool family (5) in the VibeUE upstream parity matrix.

VERIFIED
- `server.py` shows no VS Code problems after the change.

UNVERIFIED / ASSUMPTIONS
- No Unreal Editor validation run; bridge-side component ops behavior assumed correct.
- `reorder` is expected to warn/no-op (bridge limitation).

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

NEXT (Ryan)
- Run a minimal in-editor test: list components on a Blueprint, set a simple property (e.g., bool/float), and verify the Blueprint compiles.
- Validate mutation gating: confirm `SOTS_ALLOW_BPGEN_APPLY` blocks create/delete/set_property when disabled.
- Confirm `reorder` returns warning/no-op and does not reorder in SCS.

ROLLBACK
- Revert DevTools/python/sots_mcp_server/server.py
- Revert Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md
