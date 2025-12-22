# SOTS_KEM Canonical Behavior Code Pointers

Start gate (undetected + tag blocks)
- `USOTS_KEMManagerSubsystem::EvaluateStartGate` / `EvaluateStartGateInternal` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp`.
- Gate config fields: `StartGate_*` properties in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h`.

Request entry + selection
- `RequestExecution_Blessed`, `RequestExecution_WithResult`, and `RequestExecutionInternal` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp`.
- Candidate filtering: `EvaluateDefinition` / `EvaluateTagsAndHeight` in the same file.

Witness + QTE escalation
- `NotifyExecutionWitnessed` and `NotifyExecutionQTEResult` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp`.
- Delegates: `OnExecutionQTERequested` / `OnExecutionQTEResolved` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h`.

Camera contract
- `BroadcastCameraRequest` / `BroadcastCameraRelease` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp`.
- Execution definition tags: `CameraTrackTag_Primary` / `CameraTrackTag_Secondary` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_Types.h`.

Completion broadcast + TagManager signal
- `BroadcastExecutionCompleted` and `BroadcastCompletionTag` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp`.
- Completion tag config: `KEMFinishedEventTagName` / `KEMFinishedEventTagHoldSeconds` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h`.

Save blocking
- `SetSaveBlocked` + `IsSaveBlocked` in `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp` and `Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Public/SOTS_KEM_ManagerSubsystem.h`.
