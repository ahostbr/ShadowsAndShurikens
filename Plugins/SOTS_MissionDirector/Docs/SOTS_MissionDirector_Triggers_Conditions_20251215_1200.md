# SOTS MissionDirector — Triggers & Conditions (Prompt 4)

## FSOTS_MissionProgressEvent (normalized ingress)
- EventTag (FGameplayTag, canonical) — required
- InstigatorActor (TWeakObjectPtr<AActor>)
- bHasLocation + Location
- TimestampSeconds (double)
- Value01 (float scalar for gauges/counters)
- NameId (FName discriminator for item/execution/stat ids)

## Objective conditions (USOTS_ObjectiveDefinition)
- Conditions: array of FSOTS_ObjectiveCondition { RequiredEventTag, RequiredNameId (optional), RequiredCount (default 1), DurationSeconds (>0 arms a timer window), bIsFailureCondition }
- bAllConditionsRequired: true = all non-failure conditions must be satisfied, false = any one non-failure condition completes
- Completion rule: completes when non-failure conditions meet RequiredCount (and DurationSeconds if set)
- Failure rule: any satisfied bIsFailureCondition marks the objective failed (mission failure still explicit/mission-config driven)

## Runtime tracking (subsystem)
- ConditionCountsByKey and ConditionStartTimeByKey keyed as `Obj:<ObjectiveId>|Tag:<Tag>|Name:<NameId>`
- Condition timers scheduled per key when DurationSeconds > 0; verify elapsed on callback
- Runtime state maps: GlobalObjectiveStatesById, RouteStatesById, ActiveRouteId; SetObjectiveState updates timestamps and legacy completion/failure flags

## External bindings → normalized tags
- GSM: OnStealthLevelChanged → SAS.MissionEvent.GSM.StealthLevelChanged (Value01 = tier), OnAIAwarenessStateChanged → SAS.MissionEvent.GSM.AwarenessChanged (Value01 = awareness enum), OnGlobalAlertnessChanged → SAS.MissionEvent.GSM.GlobalAlertnessChanged (Value01 = alertness)
- KEM: successful execution → SAS.MissionEvent.KEM.Execution (NameId = ExecutionTag name, InstigatorActor = executor)
- Manual ingress: PushMissionProgressEvent Blueprint/C++ helper for other systems (inventory/stats/profile) until dedicated bindings are exposed

## Evaluation flow
- Progress events ignored when no active mission or mission not InProgress
- Applies to global objectives + active route objectives (default route = first authored if none chosen)
- Conditions increment counts on matching EventTag/NameId; duration conditions start timers once
- Completion: if bAllConditionsRequired, all non-failure conditions must be satisfied; otherwise any non-failure condition suffices
- Failure: any satisfied failure condition sets objective state to Failed; mission failure remains explicit/mission-config driven

## Timer behavior / limitations
- Duration conditions are validated once elapsed via timer; no continuous tick
- No invalidation logic yet (requires additional event wiring in later prompts)
- Timers are cleared on subsystem reset/mission restart; persistence/UI wiring comes in Prompt 5
