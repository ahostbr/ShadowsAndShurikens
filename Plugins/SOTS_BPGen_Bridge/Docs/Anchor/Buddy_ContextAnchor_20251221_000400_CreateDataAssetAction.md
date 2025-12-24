[CONTEXT_ANCHOR]
ID: 20251221_000400 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: CreateDataAssetAction | Owner: Buddy
Scope: Expose the new BPGen data asset builder through the bridge protocol.

DONE
- Registered a create_data_asset action that parses the FSOTS_BPGenDataAssetDef JSON payload, supports dry-run mode, and forwards requests to USOTS_BPGenBuilder::CreateDataAssetFromDef.
- Converts FSOTS_BPGenAssetResult into JSON, relays warnings/errors, and short-circuits when asset_path or asset_class_path are missing.

VERIFIED
- None (no bridge invocation or build executed).

UNVERIFIED / ASSUMPTIONS
- JSON payload parsing fully captures property arrays; builder handles all supported types at runtime.

FILES TOUCHED
- Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Submit a bridge request with asset_path=/Game/SOTS/Data/testDA.uasset pointing to SOTS_FXCueDefinition and check the response for success.
- Open /Game/SOTS/Data/testDA in the editor to confirm the FXCue Definition asset is populated.
- If failures occur, review/log builder results and extend FSOTS_BPGenAssetProperty handling.

ROLLBACK
- Remove the create_data_asset action registration and revert JSON parsing so bridge requests are ignored.
