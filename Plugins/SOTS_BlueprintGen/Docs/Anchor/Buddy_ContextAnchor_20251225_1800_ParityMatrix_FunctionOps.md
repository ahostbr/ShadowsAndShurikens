Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1800 | Plugin: SOTS_BlueprintGen | Pass/Topic: PARITY_MATRIX_FUNCTION_OPS | Owner: Buddy
Scope: Updated VibeUE parity matrix for manage_blueprint_function

DONE
- Updated parity status entries for `manage_blueprint_function` to reflect new bridge-backed `bp_function_*` support and the limited `update_properties` subset.

VERIFIED
- (none) Doc update only.

UNVERIFIED / ASSUMPTIONS
- Assumes downstream verification will confirm the new actions behave as intended.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

NEXT (Ryan)
- Validate the actions in-editor and, if confirmed, consider bumping `update_properties` from Partial → Done if expanded.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md
