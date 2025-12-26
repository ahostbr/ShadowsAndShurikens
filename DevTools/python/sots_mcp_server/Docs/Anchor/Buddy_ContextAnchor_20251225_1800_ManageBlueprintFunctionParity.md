Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1800 | Plugin: sots_mcp_server | Pass/Topic: MANAGE_BLUEPRINT_FUNCTION_PARITY | Owner: Buddy
Scope: Expanded unified MCP manage_blueprint_function to route to bp_function_* bridge actions

DONE
- `manage_blueprint_function` now supports list/get/delete, param ops, local ops, and update_properties via BPGen bridge `bp_function_*` actions.
- Mutations are guarded via `_bpgen_guard_mutation(...)` and pass `dangerous_ok=True` to align with bridge dangerous gating.
- Registered `bp_function_*` action names in `BPGEN_READ_ONLY_ACTIONS`/`BPGEN_MUTATION_ACTIONS`.

VERIFIED
- (none) No runtime/editor verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes the bridge is running and supports the new action strings.

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py

NEXT (Ryan)
- Exercise `manage_blueprint_function` actions end-to-end against a live BPGen bridge.

ROLLBACK
- Revert DevTools/python/sots_mcp_server/server.py
