# Buddy Worklog - MissionDirector Behavior Sweep 1B Law Audit

Goal: audit MissionDirector against lawfile 21.3 locks and apply minimal fixes.

Checklist findings:
1) TotalPlaySeconds persistence: FAIL -> FIXED
   - Removed Meta write in `USOTS_MissionDirectorSubsystem::BuildProfileSnapshot` (`Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp`).
   - Added ProfileShared write via `USOTS_ProfileSubsystem::BuildSnapshotFromWorld` using `GetTotalPlaySeconds` (`Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp`).
2) Save triggers compliance: PASS
   - Cadence timer only accumulates in memory (`USOTS_MissionDirectorSubsystem::HandleTotalPlaySecondsTick`).
3) TagManager boundary compliance: PASS
   - Tag reads/writes are via TagManager helpers; union-view read used in `USOTS_GameplayTagManagerSubsystem::ActorHasTag` for `HandleExecutionEvent` (`Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp`).
4) Dependency creep sanity: PASS (with rationale)
   - Removed unused include `SOTS_ProfileTypes.h` from MissionDirector cpp.
   - GSM/stealth includes remain required for `HandleStealthLevelChanged`/`HandleAIAwarenessStateChanged` mission intake.

Fixes applied:
- ProfileShared now queries MissionDirector for TotalPlaySeconds at snapshot build time (soft class path reflection).
- MissionDirector no longer writes snapshot meta directly.
- Updated Time/Rewards authority doc to reflect ProfileShared ownership of Meta write.

Deferred / known limits:
- If `SOTS_MissionDirectorSubsystem` is not loaded, `BuildSnapshotFromWorld` leaves `Meta.TotalPlaySeconds` unchanged.

Cleanup:
- Deleted Plugins/SOTS_MissionDirector/Binaries and Plugins/SOTS_MissionDirector/Intermediate.
- Deleted Plugins/SOTS_ProfileShared/Binaries and Plugins/SOTS_ProfileShared/Intermediate.
