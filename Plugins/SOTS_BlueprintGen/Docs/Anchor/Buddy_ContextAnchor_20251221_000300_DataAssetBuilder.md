[CONTEXT_ANCHOR]
ID: 20251221_000300 | Plugin: SOTS_BlueprintGen | Pass/Topic: DataAssetBuilder | Owner: Buddy
Scope: Equip BlueprintGen with BPGen payloads that can describe data assets and property overrides.

DONE
- Added FSOTS_BPGenAssetProperty / FSOTS_BPGenDataAssetDef structs to capture data asset creation inputs from JSON.
- Implemented TrySetDataAssetProperty and USOTS_BPGenBuilder::CreateDataAssetFromDef to load the requested class/package, apply overrides, and save the resulting UDataAsset.

VERIFIED
- None (no build or editor run).

UNVERIFIED / ASSUMPTIONS
- Property parsing and package save succeed at runtime and acceptable property types are covered.

FILES TOUCHED
- Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- Source/SOTS_BlueprintGen/Public/SOTS_BPGenBuilder.h
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Invoke BPGen bridge with create_data_asset payload for /Game/SOTS/Data/testDA.uasset and the FXCueDefinition class; confirm /Game/SOTS/Data contains the saved asset.
- Inspect the created FXCue Definition in editor to verify property overrides were applied.

ROLLBACK
- Revert SOTS_BPGenBuilder.* and remove the new structs if data asset creation needs to be disabled.
