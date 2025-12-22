# Buddy Worklog - Steam Behavior Sweep 12

Goal: Finish v1 Steam behavior by wiring mission/Stats events into leaderboards first, achievements second, and keep the flow resilient when Steam is unavailable.

Changes:
- Added `USOTS_SteamMissionResultBridgeSubsystem`, which watches `USOTS_MissionDirectorSubsystem::OnMissionEnded`, builds the `FSOTS_SteamMissionResult`, rate-limits duplicate posts, and skips online work when Steam is offline before handing off to achievements.
- Queried the `SOTS_Suite_ExtendedMemory_LAWs.txt:21.3.15` rule to keep leaderboards first and documented the new behavior in `SOTS_Steam_QuickStart.md`.
- Deleted `Plugins/SOTS_Steam/Binaries` and `Plugins/SOTS_Steam/Intermediate` per the cleanup rule.

Notes:
- Manual submissions can still be driven through `USOTS_SteamLeaderboardsSubsystem` / `USOTS_SteamAchievementsSubsystem` when debugging special scenarios.
