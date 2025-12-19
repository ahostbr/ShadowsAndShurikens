[CONTEXT_ANCHOR]
ID: 20251219_1308 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGen API batch fixes | Owner: Buddy
Scope: Fix UE5.7 JSON parsing, result structs, and missing helper symbols to unblock BlueprintGen compile.

DONE
- Updated TryGetObjectField usage for options/params in SOTS_BPGenAPI batch/single handling.
- Added ErrorMessage/Errors/Warnings fields to FSOTS_BPGenGraphEditResult and FSOTS_BPGenAssetResult.
- Implemented missing helper utilities (container parsing, pin def build, pin type fill, default object assignment, pin direction text) and ensured Blueprint graph modules load.
- Added knot/select node headers, link-fallback cvar, and UE5.7 variable spawner resolution changes.

VERIFIED
- None (no build/runtime executed).

UNVERIFIED / ASSUMPTIONS
- Variable spawner matching via VarProperty/owner maintains behavior parity.
- Pin type mapping suffices for existing specs.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenAPI.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h

NEXT (Ryan)
- Rebuild editor target; expect previous TryGetObjectField errors resolved.
- If variable spawner lookups fail, confirm spawner key format and adjust mapping of VarProperty/owner or relax matching.
- Validate pin type conversion for map/set pins and class/struct paths in generated graphs.

ROLLBACK
- Revert the three touched files or git revert the associated changeset if grouped.
