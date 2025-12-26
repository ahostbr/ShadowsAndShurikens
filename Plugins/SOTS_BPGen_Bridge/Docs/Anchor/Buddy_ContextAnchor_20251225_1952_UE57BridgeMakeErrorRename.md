Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1952 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UE57_MakeErrorRename_AssetRenameData | Owner: Buddy
Scope: UE 5.7.1 compile unblock for BPGen bridge helper collisions + AssetRenameData include

DONE
- Updated AssetOps include to `AssetTools/AssetRenameData.h`.
- Renamed bridge-local `MakeError(...)` helpers to avoid UE `MakeError` template collisions:
  - `MakeAssetOpError`, `MakeComponentOpError`, `MakeBlueprintOpError`.
- Updated all call sites in the three affected .cpp files.
- Removed plugin `Binaries/` and `Intermediate/` folders.

VERIFIED
- UNVERIFIED (no build run in this pass).

UNVERIFIED / ASSUMPTIONS
- Assumed UE5.7 provides `AssetTools/AssetRenameData.h` in this install.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeComponentOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeBlueprintOps.cpp

NEXT (Ryan)
- Rebuild editor target; confirm AssetOps include resolves and `TValueOrError_ErrorProxy` errors are gone.
- If include still fails, locate the correct header path in the UE install and adjust include accordingly.

ROLLBACK
- Revert the three edited bridge .cpp files.
