goal
- Deliver a BPGen graph spec that wires BeginPlay -> 3s Delay -> ShowNotification (SOTS_UI/ProHUD) for BP_BPGen_Testing at /Game/BPGen.

what changed
- Added a BPGen graph spec and apply payload for the BeginPlay notification flow targeting the EventGraph of BP_BPGen_Testing.

files changed
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_GraphSpec.json
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_ApplyPayload.json
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_181500_BPGen_BeginPlayNotification.md

notes + risks/unknowns
- The blueprint must already exist; BPGen cannot create the Blueprint asset. Assumed parent class: Actor.
- Spawner keys assumed from standard functions: ReceiveBeginPlay, Delay, GetGameInstanceSubsystem, ShowNotification. Should be validated via discovery before apply.
- Pin names (then/execute, Completed/execute, ReturnValue/self) are standard but should be confirmed by discovery.
- CategoryTag is set to SAS.UI; adjust if a different tag is desired.
- Not applied; no build or editor run performed.

verification status
- Unverified (spec authored only; not applied/compiled).

follow-ups / next steps
- Confirm BP_BPGen_Testing exists (Actor parent) at /Game/BPGen/BP_BPGen_Testing.BP_BPGen_Testing.
- Run BPGen discovery in that BP context to validate spawner keys/pin names, adjust spec if needed.
- Apply via manage_bpgen/apply_graph_spec using the provided spec/apply payload; then compile/save blueprint and verify BeginPlay triggers the notification.
