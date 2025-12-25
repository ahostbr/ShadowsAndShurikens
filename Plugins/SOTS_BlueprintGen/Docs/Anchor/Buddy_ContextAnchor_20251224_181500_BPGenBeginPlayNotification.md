Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_181500 | Plugin: SOTS_BlueprintGen | Pass/Topic: BeginPlayNotificationGraphSpec | Owner: Buddy
Scope: BPGen graph spec for BP_BPGen_Testing EventGraph wiring BeginPlay -> Delay -> ShowNotification (SOTS_UI/ProHUD).

DONE
- Authored BPGEN_BeginPlayNotification_GraphSpec.json targeting EventGraph with nodes (BeginPlay, Delay 3s, GetGameInstanceSubsystem -> UIRouter, ShowNotification message/duration/tag) and data link for self.
- Added BPGEN_BeginPlayNotification_ApplyPayload.json to call apply_graph_spec for the same blueprint/graph.
- Logged the work in BuddyWorklog_20251224_181500_BPGen_BeginPlayNotification.md.

VERIFIED
- None (spec not applied; no build/editor run).

UNVERIFIED / ASSUMPTIONS
- BP_BPGen_Testing exists at /Game/BPGen/BP_BPGen_Testing with Actor parent (required; BPGen cannot create the BP asset).
- Spawner keys/pin names for ReceiveBeginPlay, Delay, GetGameInstanceSubsystem, ShowNotification match standard signatures; discovery still needed to lock them.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_GraphSpec.json
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_ApplyPayload.json
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_181500_BPGen_BeginPlayNotification.md

NEXT (Ryan)
- Create/confirm BP_BPGen_Testing (Actor parent) at /Game/BPGen/BP_BPGen_Testing.BP_BPGen_Testing.
- Run BPGen discovery in that BP context to validate spawner keys/pins; adjust spec if discovery differs.
- manage_bpgen apply_graph_spec using the provided spec/payload, then compile/save the blueprint and verify BeginPlay shows the notification (ProHUD mirrors it).

ROLLBACK
- Delete the two example JSON files and the worklog, or revert the files listed above.
[/CONTEXT_ANCHOR]
