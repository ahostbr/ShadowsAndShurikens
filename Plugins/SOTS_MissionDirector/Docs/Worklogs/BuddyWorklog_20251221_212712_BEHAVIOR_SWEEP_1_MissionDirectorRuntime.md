# Buddy Worklog - MissionDirector Behavior Sweep 1

Goal: implement mission lifecycle spine, TotalPlaySeconds authority, objective event intake, and reward dispatch chain.

Changes:
- Added TotalPlaySeconds cadence tracking + snapshot IO (start/stop with mission run, read-only query, ProfileShared meta write).
- Added mission lifecycle determinism (state-change delegate, abort flow, end guard, route/objective state change broadcasts).
- Centralized reward dispatch into a single reward intent emission and routed rewards via SkillTree, StatsLibrary, UI router, and FX.
- Normalized progress intake so NotifyMissionEvent routes through PushMissionProgressEvent before legacy tag completion.

Notes:
- Lawfile read but not applied directly per instruction; no policy overrides introduced.
- No build/run executed.

Files touched:
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorTypes.h
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/SOTS_MissionDirector.Build.cs
- Plugins/SOTS_MissionDirector/Docs/SOTS_MissionDirector_TimeAndRewardsAuthority.md

Cleanup:
- Deleted Plugins/SOTS_MissionDirector/Binaries and Plugins/SOTS_MissionDirector/Intermediate.
