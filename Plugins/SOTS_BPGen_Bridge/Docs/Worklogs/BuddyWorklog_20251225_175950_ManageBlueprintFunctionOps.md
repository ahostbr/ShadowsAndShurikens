Send2SOTS
# Buddy Worklog — 2025-12-25 17:59:50 — BPGen Bridge: manage_blueprint_function ops

## Goal
Implement VibeUE parity item (2) `manage_blueprint_function` by adding bridge-backed `bp_function_*` actions (list/get/params/locals/delete/update_properties) and ensuring mutating ops are properly “dangerous” gated.

## What Changed
- Added bridge routing for function operations via `SOTS_BPGenBridgeFunctionOps`.
- Marked mutating function actions as dangerous (requires `params.dangerous_ok=true`; blocked by safe mode).

## Evidence Used
- RAG query report: `Reports/RAG/rag_query_20251225_174113.*`
- Upstream reference (local mirror): `WORKING/VibeUE-master/Source/VibeUE/Private/Services/Blueprint/BlueprintFunctionService.*`

## Files Changed
- `Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.h`
- `Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.cpp`
- `Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp`

## Notes / Risks / Unknowns
- `bp_function_update_properties` intentionally supports a limited subset initially (`CallInEditor`, `BlueprintPure`, `Category`). Any additional metadata support should be added carefully and verified in-editor.
- No build/editor verification performed in this pass.

## Verification
UNVERIFIED
- No build run; no in-editor validation.

## Next (Ryan)
- Build the plugin and confirm the module loads.
- Exercise each action on a known Blueprint:
  - `bp_function_list`, `bp_function_get`, `bp_function_list_params`, `bp_function_list_locals`
  - `bp_function_add_param/remove_param/update_param`
  - `bp_function_add_local/remove_local/update_local`
  - `bp_function_delete`, `bp_function_update_properties`
- Confirm safe mode blocks the mutating actions unless explicitly allowed.
