# SOTS MissionDirector — Recon (Objectives/Routes/IO)

## (1) Snapshot summary (current behavior)
- MissionDirector is a GameInstanceSubsystem that owns mission activation, objective completion flags, a scored event log, simple completion/failure checks (stealth tier + KEM + alert count + tag-based failure), and reward/tag/FX dispatch on completion/failure. No routing/branching executor beyond a data map lookup for next mission by outcome.
- Scoring/logging path is separate from mission-definition path: `StartMissionRun` tracks score/log only; `StartMission` loads a `USOTS_MissionDefinition`, initializes objective maps, and also kicks off a `StartMissionRun` using the mission id.
- Inputs are limited to GSM stealth level changes, KEM execution events, explicit mission event/tag calls, and a simple alert counter. No Inventory/Profile/UI router integration present.
- Outputs are delegates (score/event/misison state/objective), optional FX triggers, and tag applications to the player (mission started/completed/failed, reward tags/skills/abilities). Reward application is tag-only; no persistence wiring beyond stored mirrors/profile helpers.

## (2) File-by-file notes
- [Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorTypes.h](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorTypes.h): Enum definitions (mission state, objective type, completion state, event category); data structs for event log, mission summary, mission objective (design-time), runtime objective state, runtime mission state, failure conditions, rewards; DataAsset `USOTS_MissionDefinition` (mission metadata, objectives, stealth limits, FX tags, next-mission map, failure conditions, rewards, optional GSM config override).
- [Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h): GameInstanceSubsystem API, properties, delegates. Public UFUNCTIONs for mission run scoring, mission definition lifecycle, objective/tag hooks, alert counter, state export/import/profile helpers. Delegates: `OnScoreChanged`, `OnEventLogged`, `OnMissionEnded`, `OnMissionStarted`, `OnMissionCompleted`, `OnMissionFailed`, `OnObjectiveUpdated`.
- [Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp): Implementation. Subscribes to GSM `OnStealthLevelChanged` and KEM `OnExecutionEvent`; handles mission state, objective completion by tag, KEM requirements, failure conditions, alert counting, reward/FX/tag dispatch, profile mirrors. Rank evaluation is hardcoded thresholds (S/A/B/C/D).
- [Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorDebugLibrary.h](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorDebugLibrary.h) and [Private/SOTS_MissionDirectorDebugLibrary.cpp](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorDebugLibrary.cpp): Debug helpers to run a minimal mission flow and to set/check mission tags on the player via TagManager. Does not touch mission definitions.
- [Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorModule.h](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorModule.h) and [Private/SOTS_MissionDirectorModule.cpp](Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorModule.cpp): Module log category and startup/shutdown logs only.

## (3) Current DataAsset / data model
- `USOTS_MissionDefinition` (DataAsset): MissionId, MissionName/Description, MissionMap (soft world), Objectives array (`FSOTS_MissionObjective`), stealth gates (`MaxAllowedStealthTier`, `bFailOnMaxTier`), `bRequireAllMandatoryObjectives`, optional GSM config override, FX tags for start/complete/fail, `NextMissionByOutcome` map, `FSOTS_MissionFailureConditions`, `FSOTS_MissionRewards`.
- `FSOTS_MissionObjective`: ObjectiveId, Type (Mandatory/Optional) plus separate `bOptional` flag, legacy `CompletionTag` plus `CompletionTags` container, prerequisites list, `bMissionCritical`, optional KEM filters `RequiredExecutionTag` and `RequiredTargetTag`, optional Description.
- Runtime state structs: `FSOTS_MissionObjectiveRuntimeState`, `FSOTS_MissionRuntimeState`, `FSOTS_MissionRunSummary`, `FSOTS_MissionEventLogEntry` (scored log entries), `FSOTS_MissionFailureConditions`, `FSOTS_MissionRewards`.
- No DataTables observed; no route/branch table beyond `NextMissionByOutcome` map on the definition.

## (4) Current mission + objective state machines
- MissionState enum: None/Inactive/InProgress/Completed/Failed. Transitions driven manually: `StartMission` sets InProgress; `EvaluateMissionCompletion` sets Completed; `FailMission` sets Failed. No explicit reset to Inactive beyond constructor/init.
- Objective state: tracked via `ObjectiveCompletion` and `ObjectiveFailed` maps plus runtime view builder. Active flag = not completed/failed AND prerequisites met. No per-objective progress or timers.
- Completion logic: mandatory objectives gate completion based on `bRequireAllMandatoryObjectives`; if no mandatory objectives exist, completion triggers when any objective recorded.
- Failure logic: stealth gate via GSM (max tier and fail-on-max-tier), alert count limit, tag-based event failure (`FailOnEventTags`), mission-critical objective failure, and KEM execution gating for objectives. No objective-level timers/stages/phases.

## (5) Current inputs (listeners) + payloads
- GSM: binds `HandleStealthLevelChanged` to `OnStealthLevelChanged` (payload: OldLevel, NewLevel, NewScore). Uses tier to set `StealthScore` max, fail on fully detected or max tier limit. No GSM suspicion/alertness listeners.
- KEM: binds `HandleExecutionEvent` to `USOTS_KEMManagerSubsystem::OnExecutionEvent` (payload `FSOTS_KEM_ExecutionEvent`). Filters on `Result==Succeeded`, `RequiredExecutionTag`, optional `RequiredTargetTag` via TagManager, updates objective completion.
- Explicit calls: `NotifyMissionEvent` (completes objectives by tag), `CompleteObjectiveByTag`, `RegisterScoreEvent`, `RegisterObjectiveCompleted`, `NotifyAlertTriggered`, `ForceFailObjective`, `FailMission`.
- Tag subsystem pulls: `SOTS_GameplayTagManagerSubsystem` used for mission outcome tags, mission started/completed/failed tags, reward tag application, target tag checks.

## (6) Current outputs (delegates/persistence/UI intents) + payloads
- Delegates (BlueprintAssignable): `OnScoreChanged` (NewScore, Delta, ContextTags), `OnEventLogged` (FSOTS_MissionEventLogEntry), `OnMissionEnded` (FSOTS_MissionRunSummary), `OnMissionStarted`, `OnMissionCompleted`, `OnMissionFailed`, `OnObjectiveUpdated` (ObjectiveId).
- Tag writes: mission started/completed/failed tags to player; reward tags (GrantedTags, GrantedSkillTags, GrantedAbilityTags) to player; optional target tag checks (read). Uses TagManager helpers, not UI router.
- FX: triggers start/complete/fail FX tags; reward FX tag optional.
- Persistence hooks: profile mirrors stored in subsystem variables; `BuildProfileData`/`ApplyProfileData` to/from `FSOTS_MissionProfileData`; `ExportMissionState`/`ImportMissionState` and `GetCurrentMissionResult` for save/profile use. No direct save write calls present.
- No UI intents or widget routing present; no gameplay tag outputs for objectives beyond reward/outcome tags.

## (7) Unfinished / placeholder items
- Rank evaluation is hardcoded thresholds noted as "placeholder curve" in code; no data-driven tuning file (`EvaluateRankFromScore` in Subsystem cpp).
- Branching: `NextMissionByOutcome` map exists, but no executor/loader; only `GetNextMissionIdByOutcome` accessor.
- Reward handling is tag-only; no integration with ability/skill systems beyond adding tags and optional FX.
- No persistence writes: subsystem only provides mirrors; external save/profile system must call Build/Apply/Export/Import.
- No objective progress/stages; completion is binary via tags/KEM/explicit calls; optional prerequisite activation only marks bActive in runtime view (no gating of completion calls).
- Separate `bOptional` flag duplicates `ESOTS_ObjectiveType::Optional`—both kept, no consolidation.

## (8) Questions for Ryan
- Should rank evaluation be data-driven (e.g., curve table) instead of the hardcoded thresholds in `EvaluateRankFromScore`?
- Should MissionDirector trigger routing/loading of `NextMissionByOutcome` or remain a pure accessor?
- Are reward tag grants intended to be mirrored into profile saves automatically, or will another system consume `FSOTS_MissionRunSummary` to persist them?

---
Console/log summary
- Public UFUNCTION APIs (BlueprintCallable/Pure) in the subsystem: 26 (mission run + mission definition + objective hooks + save/profile accessors).
- Delegates exposed: 7 (score, event log, mission end, mission started/completed/failed, objective updated).
- External listeners bound: 2 (GSM `OnStealthLevelChanged`, KEM `OnExecutionEvent`).
- DataAssets: 1 (`USOTS_MissionDefinition`); DataTables: 0.
- Suspected legacy/separate paths: `StartMissionRun` vs `StartMission` split; legacy `CompletionTag` retained alongside `CompletionTags`.
