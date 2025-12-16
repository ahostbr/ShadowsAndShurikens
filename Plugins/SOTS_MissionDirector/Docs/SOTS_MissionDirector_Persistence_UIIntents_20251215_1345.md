# SOTS MissionDirector — Persistence & UI Intents (Prompt 5)

## Milestone write points (no spam)
- Mission start (StartMission after runtime init)
- Route activation (default route activation during mission init; future route switches hook same path)
- Objective terminal states: Completed / Failed
- Mission terminal states: Completed / Failed / Aborted

## Snapshot (FSOTS_MissionMilestoneSnapshot)
- MissionId (FSOTS_MissionId, falls back to legacy MissionId)
- MissionState (ESOTS_MissionState)
- ActiveRouteId (FSOTS_RouteId)
- CompletedObjectives (array of FSOTS_ObjectiveId)
- FailedObjectives (array of FSOTS_ObjectiveId)
- TimestampSeconds (double)

Build helper: `BuildMilestoneSnapshot(NowSeconds)` collects global + route objective runtime states. Write helper: `TryWriteMilestoneToStatsAndProfile(Snapshot)` (safe no-op if sinks missing, caches history).

## UI intents (via SOTS_UIRouterSubsystem only)
- MissionStarted → CategoryTag: SAS.UI.Mission.MissionStarted (Title from MissionDefinition or fallback)
- RouteActivated → SAS.UI.Mission.RouteActivated (includes route id in message)
- ObjectiveCompleted → SAS.UI.Mission.ObjectiveCompleted (Title from ObjectiveDefinition when available)
- ObjectiveFailed → SAS.UI.Mission.ObjectiveFailed
- MissionCompleted → SAS.UI.Mission.MissionCompleted
- MissionFailed → SAS.UI.Mission.MissionFailed
- MissionAborted → SAS.UI.Mission.MissionAborted (future-proof)

Payload path: uses `PushNotification_SOTS` with F_SOTS_UINotificationPayload (Message, DurationSeconds=2.5, CategoryTag). No widgets created.

## Missing-subsystem behavior
- Stats/Profile persistence sink missing: milestone is cached locally; one-time verbose log only.
- UIRouter missing: intent is skipped with one-time verbose log; no crash.

## Tags added (DefaultGameplayTags.ini)
- Progress: SAS.MissionEvent.GSM.StealthLevelChanged, SAS.MissionEvent.GSM.AwarenessChanged, SAS.MissionEvent.GSM.GlobalAlertnessChanged, SAS.MissionEvent.KEM.Execution
- UI: SAS.UI.Mission.MissionStarted, SAS.UI.Mission.RouteActivated, SAS.UI.Mission.ObjectiveCompleted, SAS.UI.Mission.ObjectiveFailed, SAS.UI.Mission.MissionCompleted, SAS.UI.Mission.MissionFailed, SAS.UI.Mission.MissionAborted, SAS.UI.Mission.MissionUpdated
