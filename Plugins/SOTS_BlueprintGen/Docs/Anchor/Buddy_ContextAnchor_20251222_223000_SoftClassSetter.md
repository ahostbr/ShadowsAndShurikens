[CONTEXT_ANCHOR]
ID: 20251222_223000 | Plugin: SOTS_BlueprintGen | Pass/Topic: SoftClassSetter | Owner: Buddy
Scope: Fix data asset overrides so soft-class properties are stored with the expected FSoftObjectPtr.

DONE
- Switched the soft-class override branch in `TrySetDataAssetProperty` to construct an `FSoftObjectPtr` from the provided `FSoftObjectPath` so the setter receives the type the reflection API actually expects.
- Rebuilt ShadowsAndShurikensEditor to confirm the SOTS_BlueprintGen/SOTS_BPGen_Bridge modules now compile cleanly.

VERIFIED
- ShadowsAndShurikensEditor now builds successfully following the soft-class refactor; UnrealBuildTool logged a successful `Build.bat` run.

UNVERIFIED / ASSUMPTIONS
- The create_data_asset bridge workflow still needs to run to prove that the corrected soft-class handling works at runtime.

FILES TOUCHED
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Invoke the BPGen bridge `create_data_asset` action for `/Game/SOTS/Data/testDA.uasset` targeting the FXCueDefinition class and confirm the saved asset contains the expected overrides.
- Open the asset in the editor (or inspect the saved uasset) to ensure the soft-class property uses the intended soft reference.

ROLLBACK
- Revert Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp if the soft-class fix causes regressions in other data asset scenarios.
