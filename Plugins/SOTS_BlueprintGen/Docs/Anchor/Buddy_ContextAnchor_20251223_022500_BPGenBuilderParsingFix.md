[CONTEXT_ANCHOR]
ID: 20251223_022500 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenBuilderParsingFix | Owner: Buddy
Scope: Replace missing-library parsing helpers with std-based logic so UE 5.7 builds succeed.

DONE
- Trimmed `FillPinTypeFromBPGen` to only accept valid categories and preserved container/object handling without undefined temporaries.
- Simplified `TryResolvePinAlias` to bail out when alias context is missing and to publish canonical names when found.
- Added `<cstdlib>`/`FTCHARToUTF8` support and rewrote the float/int string parsers to use `std::strtof`/`std::strtoll` with strict full-string consumption.
- Implemented the `TrySetDataAssetProperty` helper so string-based overrides are applied via the type-safe reflection helpers and the data-asset builder compiles.

VERIFIED
- None (no build or editor run after these edits).

UNVERIFIED / ASSUMPTIONS
- These helpers now compile against the standard library APIs and behave like the old `LexTryParseString` variants.
- Bridge requests will still reach `CreateDataAssetFromDef` once the modules rebuild.

FILES TOUCHED
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Run the UE 5.7 build (SOTS_BlueprintGen/SOTS_BPGen_Bridge) so the new parsing helpers are compiled and registered.
- After the build, run the bridge `create_data_asset` payload for `/Game/SOTS/Data/testDA.uasset` and verify the FXCue Definition asset.

ROLLBACK
- Revert `Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp` to the prior state where parsing relied on engine helpers.
