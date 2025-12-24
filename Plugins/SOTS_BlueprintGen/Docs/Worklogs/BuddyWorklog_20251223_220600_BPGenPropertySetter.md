goal
- Resolve the missing `TrySetDataAssetProperty` definition so the new data asset builder compiles and fully processes property overrides.

what changed
- Implemented `TrySetDataAssetProperty` next to the other parsing helpers so string overrides for bools, ints, floats, text, names, enums, objects, and soft references can be applied via reflection.
- Updated the BPGen parsing-anchor doc to call out the helper addition for this pass.

files changed
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Docs/Anchor/Buddy_ContextAnchor_20251223_022500_BPGenBuilderParsingFix.md

notes + risks/unknowns
- Build hasn’t been rerun; the new helper may still miss some property flavors or path formats.
- Property strings now rely on `StaticLoadObject`, so asset references must be valid or the bridge will report errors.

verification status
- UNVERIFIED (no UE build or bridge invocation performed after the edit).

follow-ups / next steps
- Rebuild SOTS_BlueprintGen/SOTS_BPGen_Bridge so UE 5.7 has the new helper definitions and the bridge can run again.
- Retry the `create_data_asset` request from the bridge once the modules compile. 