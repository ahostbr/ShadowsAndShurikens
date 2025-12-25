goal
- Plan BPGen steps to create /Game/BPGen/BP_BPGen_Testing with a BeginPlay-driven notification flow (delay then UI/ProHUD notification).

what changed
- Planning only; documented the BPGen action plan in chat and captured it here. No repo assets or code were modified.

files changed
- Plugins/SOTS_BlueprintGen/Docs/Worklogs/BuddyWorklog_20251224_180000_BPGen_TestNotification.md

notes + risks/unknowns
- Spawner keys for BeginPlay, Delay, and USOTS_UIRouterSubsystem::ShowNotification must be confirmed via BPGen discovery before applying a GraphSpec.
- Target Blueprint base class not specified; assuming an Actor unless clarified.
- Notification CategoryTag choice assumed to use an existing tag (e.g., SAS.UI) to avoid introducing new schema entries.
- Application requires BPGen actions (discover → apply) in-editor/commandlet; not executed yet.

verification status
- Unverified (plan-only; no apply/build/run performed).

follow-ups / next steps
- Run BPGen discovery against /Game/BPGen/BP_BPGen_Testing (or a staging BP) to capture spawner keys for Event BeginPlay, KismetSystemLibrary Delay, and SOTS_UIRouterSubsystem::ShowNotification.
- Decide Blueprint parent class (Actor vs specific base) and confirm asset path creation is allowed in the BPGen runner.
- Build a GraphSpec (Entry → Delay 3s → ShowNotification message "Merry Christmas Ryan !" duration/category) and apply via manage_bpgen/apply_graph_spec.
- Verify in-editor that BeginPlay triggers the notification and that the ProHUD adapter mirrors it.
