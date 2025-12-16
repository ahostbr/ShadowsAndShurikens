# SOTS MissionDirector — Data Schema Contract (Prompt 2)

## Mission / Route / Objective DataAssets
- Mission (USOTS_MissionDefinition): Golden-path `MissionIdentifier` (FSOTS_MissionId), title/description, routes array (USOTS_RouteDefinition), global objectives array (USOTS_ObjectiveDefinition), legacy inline objectives kept, mission fail enable, existing stealth/failure/reward fields preserved.
- Route (USOTS_RouteDefinition): `RouteId` (FSOTS_RouteId), title/description, objectives array (USOTS_ObjectiveDefinition). No trigger logic yet.
- Objective (USOTS_ObjectiveDefinition): `ObjectiveId` (FSOTS_ObjectiveId), title/description, `bIsOptional`, AllowedRoutes, RequiresCompleted, `bCanFail` placeholder. Legacy FSOTS_MissionObjective remains for existing content.
- ID rule: Id fields are FName; uniqueness is required per mission. AllowedRoutes/RequiresCompleted must reference known Ids.

## Runtime state structs
- FSOTS_ObjectiveRuntimeState: ObjectiveId, ObjectiveState (Inactive/Active/Completed/Failed/Abandoned), Activated/Completed/Failed timestamps.
- FSOTS_RouteRuntimeState: RouteId, bIsActive, ObjectiveStatesById (map keyed by ObjectiveId.Id).
- FSOTS_MissionRuntimeState: MissionId, MissionState (NotStarted/InProgress/Completed/Failed/Aborted), ActiveRouteId, GlobalObjectiveStatesById, RouteStatesById, StartTimeSeconds, EndTimeSeconds, plus legacy summary fields kept for compatibility.

## Delegate payload structs (stable IO)
- FSOTS_MissionStateChangedPayload: MissionId, OldState, NewState, TimestampSeconds.
- FSOTS_ObjectiveStateChangedPayload: MissionId, RouteId, bIsGlobalObjective, ObjectiveId, OldState, NewState, TimestampSeconds.
- FSOTS_RouteActivatedPayload: MissionId, RouteId, TimestampSeconds.
- Delegates declared (no wiring yet): FSOTS_OnMissionStateChanged, FSOTS_OnObjectiveStateChanged, FSOTS_OnRouteActivated.

## Notes
- This prompt defines contracts only: no trigger evaluation, persistence, or UI wiring yet.
- Legacy fields/classes remain; new DataAssets/IDs are the golden path going forward.
- Validation helper added: ValidateMissionDefinition checks unique route/objective ids and reference integrity.
