Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_201751 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeBlueprintCreate | Owner: Buddy
Scope: Teach the bridge to accept create_blueprint_asset requests and surface builder results.

DONE
- Added a create_blueprint_asset branch in SOTS_BPGenBridgeServer that translates the JSON params into an FSOTS_BPGenBlueprintDef and forwards it to USOTS_BPGenBuilder::CreateBlueprintAssetFromDef.
- Marshaled FSOTS_BPGenBlueprintResult back to the client (result JSON, warnings, errors) so MCP consumers can see success/failure details.

VERIFIED
- None (request timed out because the bridge process is currently unreachable).

UNVERIFIED / ASSUMPTIONS
- Bridge needs to be rebuilt/restarted before the new handler is live; the current runtime still rejects create_blueprint_asset.
- Once the bridge is online, bpgen_create_blueprint + the BeginPlay notification spec should be able to create BP_BPGen_Testing and wire the notification.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- DevTools/python/logs/sots_mcp_server.jsonl

NEXT (Ryan)
- Build/deploy the updated SOTS_BPGen_Bridge plugin (restart UE/bridge server) so the new action is registered.
- Run the create_blueprint_asset job with SOTS_ALLOW_APPLY=1 and confirm /Game/BPGen/BP_BPGen_Testing is created.
- Apply BPGEN_BeginPlayNotification_ApplyPayload.json and verify the "Merry Christmas Ryan !" notification appears via SOTS UI/ProHUD.

ROLLBACK
- Revert Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp to the previous commit (git checkout -- <path>).
