Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_000900 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenTesting | Owner: Buddy
Scope: Update the BeginPlay → Delay → ShowNotification graph spec so GetUIRouter uses K2Node_GetSubsystem and run the BPGen job again until the spec applies cleanly.

DONE
- Reworked Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_GraphSpec.json to spawn a K2Node_GetSubsystem node that targets /Script/SOTS_UI.SOTS_UIRouterSubsystem (CustomClass ExtraData) instead of the previous GetGameInstanceSubsystem call.
- Ran WORKING/run_bpgen_notification_job.py with .venv_mcp (SOTS_ALLOW_BPGEN_APPLY=1) to recreate /Game/BPGen/BP_BPGen_Testing, apply the new graph spec, and record the successful bpgen_create_blueprint, apply_function_skeleton, and apply_graph_spec responses.

VERIFIED
- BPGen bridge log entries (create/apply) under DevTools/python/logs/sots_mcp_server.jsonl show each action completed without Spawner/MissingPin warnings after the spec change.

UNVERIFIED / ASSUMPTIONS
- The BeginPlay-powered notification has not been observed in the editor or runtime; the actor still needs to be spawned to confirm the alert surfaces through both SOTS UI and ProHUD.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_GraphSpec.json
- Content/BPGen/BP_BPGen_Testing.uasset

NEXT (Ryan)
- Inspect /Game/BPGen/BP_BPGen_Testing in-editor to verify the K2Node_GetSubsystem wiring, compile the graph, and ensure the Delay → ShowNotification flow is intact.
- Play the actor (BeginPlay) in PIE to confirm the “Merry Christmas Ryan !” banner appears via SOTS UI/ProHUD.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_GraphSpec.json to the prior spec and delete Content/BPGen/BP_BPGen_Testing.uasset (or rerun the job with the previous spec) if this change needs to be undone.
[/CONTEXT_ANCHOR]
