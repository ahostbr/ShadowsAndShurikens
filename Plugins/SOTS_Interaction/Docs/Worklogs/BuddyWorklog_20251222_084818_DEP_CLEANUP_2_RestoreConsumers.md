# Buddy Worklog - DEP_CLEANUP_2_RestoreConsumers (20251222_084818)

Prompt
- [SOTS_VSCODE_BUDDY] category: build_fix, plugin: SOTS_Suite, pass: DEP_CLEANUP_2_RESTORE_VERB_CONSUMERS, ts: 2025-12-22 08:35 America/New_York

Goal
- Keep Interaction as emitter-only while restoring Execute + DragStart/DragStop handling via subscriber modules.
- Break plugin/module cycles by moving verb consumers into KEM/BodyDrag listeners.

What changed
- Interaction now treats DragStart/DragStop as canonical cross-plugin verbs again (no direct KEM/BodyDrag calls) and still emits OnInteractionActionRequested payloads.
- BodyDrag added GameInstance subsystem bridge to subscribe to Interaction action requests and drive TryBeginDrag/TryDropBody on the player component; default option tags now use canonical Interaction.Verb.* for drag verbs.
- KEM now subscribes to Interaction action requests and routes Execute verbs through RequestExecution_Blessed; added dependency wiring (Build.cs + .uplugin).
- Ran DevTools depmap (DevTools/reports/sots_plugin_depmap.json) to confirm dependencies without Interaction↔BodyDrag/KEM cycles.

Files changed
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Plugins/SOTS_Interaction/SOTS_Interaction.uplugin
- Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Private/SOTS_BodyDragInteractableComponent.cpp
- Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Private/SOTS_BodyDragInteractionBridgeSubsystem.h (new)
- Plugins/SOTS_BodyDrag/Source/SOTS_BodyDrag/Private/SOTS_BodyDragInteractionBridgeSubsystem.cpp (new)
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/SOTS_KillExecutionManager.Build.cs
- Plugins/SOTS_KillExecutionManager/SOTS_KillExecutionManager.uplugin

Notes / decisions
- Interaction remains dependency-free on KEM/BodyDrag; consumers bind to OnInteractionActionRequested.
- BodyDrag bridge binding: USOTS_BodyDragInteractionBridgeSubsystem::HandleInteractionActionRequested() (DragStart -> TryBeginDrag, DragStop -> TryDropBody).
- KEM binding: USOTS_KEMManagerSubsystem::HandleInteractionActionRequested() (Execute verb -> RequestExecution_Blessed).
- Depmap warns about BOM on some .uplugin files but still shows KEM depending on Interaction; JSON at DevTools/reports/sots_plugin_depmap.json.

Verification
- Not run (per Token Guard); dependency map script executed for structural confirmation only.

Cleanup
- Deleted Binaries/ and Intermediate/ for SOTS_Interaction, SOTS_BodyDrag, SOTS_KillExecutionManager.

Follow-ups / TODOs
- Remove BOM from affected .uplugin files so depmap parses without warnings.
- Confirm existing BodyDrag assets use canonical Interaction.Verb.DragStart/DragStop tags so subscriber path is exercised.
- Monitor for missing ExecutionTag on Execute verbs; add fallback source if real data lacks tags.
