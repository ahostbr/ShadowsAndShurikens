# Buddy Worklog - DEP_CLEANUP_2A_Followups (20251222_085534)

Prompt
- [SOTS_VSCODE_BUDDY] category: build_fix, plugin: SOTS_Suite, pass: DEP_CLEANUP_2A_FOLLOWUPS, ts: 2025-12-22 09:18 America/New_York

Goal
- Harden the Interaction emitter → BodyDrag/KEM consumer flow post DEP_CLEANUP_2: remove BOM warnings, ensure travel-safe bindings, and add Execute fallback.

What changed
- Removed UTF-8 BOMs from Interaction, BodyDrag, KEM, and AIPerception .uplugin files for clean depmap parsing.
- Re-ran depmap (DevTools/reports/sots_plugin_depmap.json); no warnings, and dependency directions remain one-way (Interaction emitter only).
- BodyDrag GI bridge now guards double-binding, retries binding via timer until Interaction is present, and unbinds/clears timers on deinit (travel-safe, no tick).
- KEM subscriber now logs-once and falls back to internal selection/start-gate path when an Execute verb arrives without an ExecutionTag; still prefers blessed path when tag is present.
- Drag verb tags already present in DefaultGameplayTags.ini; no tag additions required.

Files changed
- Plugins/SOTS_Interaction/SOTS_Interaction.uplugin
- Plugins/SOTS_BodyDrag/SOTS_BodyDrag.uplugin
- Plugins/SOTS_KillExecutionManager/SOTS_KillExecutionManager.uplugin
- Plugins/SOTS_AIPerception/SOTS_AIPerception.uplugin
- Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Private/SOTS_BodyDragInteractionBridgeSubsystem.h
- Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Private/SOTS_BodyDragInteractionBridgeSubsystem.cpp
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp

Notes / decisions
- BodyDrag still requires a Public dependency on SOTS_Interaction because its public component derives from ISOTS_InteractableInterface (cannot forward-declare UINTERFACE cleanly without exposing that type).
- Retry timer is 2s, log-once on first failure, clears after successful bind to avoid travel spam.
- KEM fallback uses RequestExecution (full start-gate/selection) with provided ContextTags; blessed path remains for tagged Execute verbs.

Verification
- No builds/run per Token Guard.
- Depmap executed successfully (no BOM warnings): DevTools/reports/sots_plugin_depmap.json.

Cleanup
- Deleted Binaries/Intermediate for Interaction, BodyDrag, KEM, AIPerception.

Follow-ups / TODOs
- If future BodyDrag API surface changes, re-evaluate whether SOTS_Interaction can move to Private deps.
- Monitor logs for missing ExecutionTag warnings; source the tag upstream if seen frequently.
