Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_184402 | Plugin: SOTS_BlueprintGen | Pass/Topic: BeginPlayNotification | Owner: Buddy
Scope: Document a BeginPlay notification example that demonstrates Blueprint creation + SOTS UI/ProHUD output.

DONE
- Added BPGEN_BeginPlayNotification_BlueprintCreateJob.json so BPGen can exercise the new create_blueprint_asset action before applying the graph spec.
- Expanded Examples/README.md and ExpectedOutcomes.md to describe the BeginPlay notification flow and its expected "Merry Christmas Ryan !" result.

VERIFIED
- UNVERIFIED (not run).

UNVERIFIED / ASSUMPTIONS
- The Blueprint creation job uses the new factory path and assumes /Game/BPGen exists.
- The expectation that ShowNotification surfaces through ProHUDV2 relies on SOTS_UI behavior and has not been run in-editor.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_184402_BPGenNotification.md
- Plugins/SOTS_BlueprintGen/Docs/Anchor/Buddy_ContextAnchor_20251224_184402_BPGenNotification.md
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_BlueprintCreateJob.json
- Plugins/SOTS_BlueprintGen/Examples/README.md
- Plugins/SOTS_BlueprintGen/Examples/ExpectedOutcomes.md

NEXT (Ryan)
- Run the new create_blueprint_asset example with SOTS_ALLOW_APPLY=1 to confirm the asset is created.
- Apply BPGEN_BeginPlayNotification_ApplyPayload.json and open the generated Blueprint to ensure the BeginPlay graph matches the spec.

ROLLBACK
- `git checkout -- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_BlueprintCreateJob.json Plugins/SOTS_BlueprintGen/Examples/README.md Plugins/SOTS_BlueprintGen/Examples/ExpectedOutcomes.md Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_184402_BPGenNotification.md Plugins/SOTS_BlueprintGen/Docs/Anchor/Buddy_ContextAnchor_20251224_184402_BPGenNotification.md`
