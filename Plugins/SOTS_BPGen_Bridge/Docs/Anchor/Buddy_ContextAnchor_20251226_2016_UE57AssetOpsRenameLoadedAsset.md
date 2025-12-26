Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_2016 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UE57_AssetOps_RenameLoadedAsset | Owner: Buddy
Scope: UE5.7 build unblock for AssetOps texture rename path

DONE
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp: removed `AssetTools/AssetRenameData.h` dependency
- ImportTexture rename: replaced `FAssetRenameData` + `IAssetTools::RenameAssets` with `UEditorAssetLibrary::RenameLoadedAsset`
- Added warning emission when rename fails (import still succeeds)
- Deleted Plugins/SOTS_BPGen_Bridge/Binaries and Plugins/SOTS_BPGen_Bridge/Intermediate

VERIFIED
- None (no build/editor run)

UNVERIFIED / ASSUMPTIONS
- Destination asset path format for `RenameLoadedAsset` is acceptable as `/Game/.../<Name>`

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp

NEXT (Ryan)
- Build `ShadowsAndShurikensEditor` and confirm the `AssetRenameData` include error is resolved.
- If compile proceeds, continue log-driven blocker iteration.
- If rename path format is rejected, adjust to the exact expected format for `UEditorAssetLibrary` APIs.

ROLLBACK
- Revert Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp
