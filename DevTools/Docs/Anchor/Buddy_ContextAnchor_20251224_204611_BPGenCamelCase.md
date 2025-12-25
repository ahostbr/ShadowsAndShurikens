Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_2046 | Plugin: DevTools | Pass/Topic: BPGenCamelCasePayload | Owner: Buddy
Scope: Align the MCP `bpgen_create_blueprint` tool + notification job with the bridge’s CamelCase payload expectations.

DONE
- bpgen_create_blueprint now sends both snake_case and CamelCase keys before routing through `_bpgen_call`, avoiding the previous "Blueprint asset path is required" failure.
- WORKING/run_bpgen_notification_job.py now invokes `apply_function_skeleton` (TargetBlueprintPath + FunctionName) before applying the BeginPlay graph, and the job was re-run to capture current bridge behavior.

VERIFIED
- create_blueprint_asset reports `/Game/BPGen/BP_BPGen_Testing` exists instead of rejecting the payload; the MCP log shows the camel-case request reached the bridge.

UNVERIFIED / ASSUMPTIONS
- apply_function_skeleton still needs to be built into the running SOTS_BPGen_Bridge binary, so the notification job still cannot wire EventGraph until the bridge restart.

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- WORKING/run_bpgen_notification_job.py
- DevTools/python/logs/sots_mcp_server.jsonl

NEXT (Ryan)
- Rebuild and restart the SOTS_BPGen_Bridge plugin so it can honor the new apply_function_skeleton handler, then rerun the BPGen notification job with SOTS_ALLOW_APPLY=1.
- Confirm the BeginPlay → Delay → ShowNotification graph applies inside BP_BPGen_Testing and surfaces the “Merry Christmas Ryan !” notification via SOTS UI/ProHUD.

ROLLBACK
- Revert the DevTools server/script files and logs touched above if the camel-case change or job script fails validation.
