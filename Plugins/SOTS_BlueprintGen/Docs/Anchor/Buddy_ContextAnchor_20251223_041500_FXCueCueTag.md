[CONTEXT_ANCHOR]
ID: 20251223_041500 | Plugin: SOTS_BlueprintGen | Pass/Topic: CueTagOverride | Owner: Buddy
Scope: Make the data asset builder understand `FGameplayTag` overrides (CueTag) so BPGen can stamp cues programmatically.

DONE
- Added `FStructProperty` handling to `TrySetDataAssetProperty` so `FGameplayTag` strings are resolved and copied into the struct value instead of falling through to the unsupported-type error.
- Rebuilt the editor and reran the completed `create_data_asset` flow while the bridge was running, including the `CueTag=Test.Test` override to confirm the new path.

VERIFIED
- `create_data_asset` returned `bSuccess` for `/Game/SOTS/Data/testDA.uasset` with the CueTag override after the `SOTS_BPGen_Bridge` process was started via the editor.

UNVERIFIED / ASSUMPTIONS
- The saved asset still needs to be opened in-editor to confirm the `CueTag` and other overrides applied correctly.

FILES TOUCHED
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- (bridge call + build log)

NEXT (Ryan)
- Open `/Game/SOTS/Data/testDA.uasset` in the editor to verify the `CueTag` and any other overrides.
- If the tag override is correct, consider adding a regression test or automating the cue-building workflow to exercise gameplay tag overrides in future PRs.

ROLLBACK
- Revert `Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp` if `FGameplayTag` handling introduces regressions in other overrides.
