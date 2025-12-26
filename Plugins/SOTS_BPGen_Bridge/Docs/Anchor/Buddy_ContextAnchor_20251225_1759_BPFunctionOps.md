Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1759 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BP_FUNCTION_OPS | Owner: Buddy
Scope: Added bridge actions backing VibeUE manage_blueprint_function parity

DONE
- Added `SOTS_BPGenBridgeFunctionOps` bridge service with actions:
  - Read: `bp_function_list`, `bp_function_get`, `bp_function_list_params`, `bp_function_list_locals`
  - Mutations: `bp_function_delete`, `bp_function_add_param`, `bp_function_remove_param`, `bp_function_update_param`, `bp_function_add_local`, `bp_function_remove_local`, `bp_function_update_local`, `bp_function_update_properties`
- Routed the above actions in the BPGen bridge dispatcher.
- Marked mutating `bp_function_*` actions as dangerous (safe-mode gated; requires `dangerous_ok=true`).

VERIFIED
- (none) No build/editor verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes the limited `update_properties` subset is sufficient for initial parity needs.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Build + load plugin in-editor.
- Validate read actions return correct function/param/local inventories.
- Validate mutations apply correctly and are blocked by safe mode unless explicitly permitted.

ROLLBACK
- Revert the files listed above; rebuild to regenerate plugin binaries.
