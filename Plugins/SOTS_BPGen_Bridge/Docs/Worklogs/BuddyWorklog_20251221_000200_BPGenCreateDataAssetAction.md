goal
- Wire the bridge to the new BPGen data asset builder so external clients can request asset creation.

what changed
- Added a create_data_asset action in SOTS_BPGenBridgeServer that parses the FSOTS_BPGenDataAssetDef payload, handles dry-run, and calls USOTS_BPGenBuilder::CreateDataAssetFromDef.
- Converted FSOTS_BPGenAssetResult to JSON and propagated any errors/warnings back to the client.

files changed
- Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

notes + risks/unknowns
- No tests have run; asset creation may fail for invalid class/package paths.
- JSON payload validation is minimal; malformed properties may silently skip.

verification status
- UNVERIFIED (no bridge invocation executed).

follow-ups / next steps
- Compose BPGen request data that sets asset_path=/Game/SOTS/Data/testDA.uasset and asset_class_path=/Game/SOTS/FXCueDefinition.SOTS_FXCueDefinition_C.
- Run apply_graph_spec_to_target against the BPGen bridge to confirm the data asset is created.
