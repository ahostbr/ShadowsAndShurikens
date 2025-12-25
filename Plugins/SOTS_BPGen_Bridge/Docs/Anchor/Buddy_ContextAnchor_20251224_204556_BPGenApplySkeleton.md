Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_2045 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BPGenApplySkeleton | Owner: Buddy
Scope: Add an `apply_function_skeleton` action handler so BPGen jobs can create EventGraphs before wiring nodes.

DONE
- Routed `apply_function_skeleton` JSON params through `FSOTS_BPGenFunctionDef`, invoked `USOTS_BPGenBuilder::ApplyFunctionSkeleton`, and serialized the `FSOTS_BPGenApplyResult` back to the requester.
- Documented the rerun of WORKING/run_bpgen_notification_job.py after the build attempt; the bridge currently rejects the new action until the binary is rebuilt, so `apply_graph_spec` still fails.

VERIFIED
- create_blueprint_asset now reports `/Game/BPGen/BP_BPGen_Testing` exists and the bridge shows the camel-case request in the MCP logs, proving the bridge is running and listening.

UNVERIFIED / ASSUMPTIONS
- The new `apply_function_skeleton` handler will work once the plugin is rebuilt/restarted; nothing has been verified yet because the running bridge still returns "Unknown action".

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- DevTools/python/logs/sots_mcp_server.jsonl

NEXT (Ryan)
- Rebuild/redeploy SOTS_BPGen_Bridge so `apply_function_skeleton` is served, rerun the BPGen notification job, and confirm the BeginPlay → Delay → ShowNotification graph applies and surfaces the “Merry Christmas Ryan !” notification via SOTS UI/ProHUD.

ROLLBACK
- Revert the bridge server edit if the new action causes regressions or mis-parses the JSON params.
