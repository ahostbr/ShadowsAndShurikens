# SOTS MissionDirector Time + Rewards Authority

## TotalPlaySeconds ownership (MissionDirector only)
- Time accumulation starts in `USOTS_MissionDirectorSubsystem::StartMissionRun` which invokes `StartTotalPlaySecondsTimer` (`Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp`).
- Cadenced accumulation occurs in `HandleTotalPlaySecondsTick` (1.0s cadence via `kTotalPlaySecondsCadence`) and is gated by `bMissionActive` + `MissionState == InProgress`.
- Mission terminal paths (`EvaluateMissionCompletion`, `FailMission`, `AbortMission`) route through `EndMissionRun`, which calls `StopTotalPlaySecondsTimer`.
- Read-only query surface: `USOTS_MissionDirectorSubsystem::GetTotalPlaySeconds`.
- Snapshot IO: ProfileShared `USOTS_ProfileSubsystem::BuildSnapshotFromWorld` queries `GetTotalPlaySeconds` and writes `FSOTS_ProfileSnapshot.Meta.TotalPlaySeconds`; `USOTS_MissionDirectorSubsystem::ApplyProfileSnapshot` restores into `TotalPlaySeconds`.

## Reward dispatch chain (MD -> SkillTree -> Stats -> UI)
- Rewards are dispatched once per completion in `EvaluateMissionCompletion` by calling `DispatchMissionRewards` (guarded by `bRewardsDispatched`).
- `DispatchMissionRewards` emits `OnMissionRewardIntent` with `FSOTS_MissionRewardIntent` (versioned payload).
- Tag grants remain via `USOTS_GameplayTagManagerSubsystem::AddTagToActor` for `GrantedTags`, `GrantedSkillTags`, and `GrantedAbilityTags`.
- Skill points: `USOTS_SkillTreeSubsystem::AddSkillPoints` for `RewardSkillTreeIds` (falls back to the sole registered tree if exactly one is registered).
- Stats deltas: `USOTS_StatsLibrary::AddToActorStat` for `StatDeltas` (positive deltas gated by `USOTS_SkillTreeSubsystem::CanRaiseStat` when available).
- UI intent: `EmitMissionUIIntent` sends a single `SAS.UI.Mission.MissionUpdated` notification with "Mission rewards granted."
- FX: optional `USOTS_FXManagerSubsystem::TriggerFXByTag` using `FXTag_OnRewardsGranted`.
