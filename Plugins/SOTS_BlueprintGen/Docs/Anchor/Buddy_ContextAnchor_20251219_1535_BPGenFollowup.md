[CONTEXT_ANCHOR]
ID: 20251219_1535 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenFollowup | Owner: Buddy
Scope: Address new BPGen compile errors (headers, SpawnSelectNode) after latest UBT log.

DONE
- Set SOTS_BlueprintGen module Type to Editor in Build.cs for editor header access.
- Removed unused Toolkits/AssetEditorManager include from SOTS_BPGenDebug.cpp.
- Fixed SpawnSelectNode definition mismatch (removed template to match declaration).
- Deleted SOTS_BlueprintGen Binaries/Intermediate post-edit.

VERIFIED
- None; compile not rerun.

UNVERIFIED / ASSUMPTIONS
- Expect EdGraphSchema_K2 include to resolve with Editor-only module flag; spawn select linkage now satisfied.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDebug.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Re-run UBT; capture any remaining missing-header or linkage errors.
- If EdGraphSchema_K2 still missing, inspect engine include paths or gate code with WITH_EDITOR guards as needed.

ROLLBACK
- Revert the three touched files; restore plugin Binaries/Intermediate from VCS/backups if necessary.
