# SOTS_Steam QuickStart

SOTS_Steam is a runtime plugin that adds Steam-style Achievements and Leaderboards to Shadows And Shurikens (SOTS), built on top of Unreal's OnlineSubsystemSteam. Local SaveGames remain the source of truth; Steam is treated as an optional mirror.

---

## 1. Enabling the Plugin

1. Enable **SOTS_Steam** in the Plugins window.
2. Restart the editor if prompted.

(Optional) Configure OnlineSubsystemSteam in your project's DefaultEngine.ini:
- Ensure the `OnlineSubsystemSteam` module is enabled (`bEnabled=true`).
- Provide the proper Steam AppId and any other Steam settings the subsystem expects.

---

## 2. Configure SOTS_Steam Settings

Open **Project Settings → SOTS → Steam Integration**.

Key options:

- **Enable Achievements**  
  Enables local achievement tracking via `USOTS_SteamAchievementsSubsystem`.

- **Enable Leaderboards**  
  Enables local leaderboard tracking via `USOTS_SteamLeaderboardsSubsystem`.

- **Enable Steam Integration**  
  If true, SOTS_Steam will talk to OnlineSubsystemSteam (if active) and mirror local progress to Steam.

- **Use Steam For Achievements / Leaderboards**  
  Per-feature toggles controlling whether each work unit mirrors to Steam.

- **Verbose Logging**  
  Prints extra details to the log for debugging (achievement state, leaderboard rows, etc.).

---

## 3. Defining Achievements and Leaderboards

Achievements and leaderboards are defined in registries:

- `USOTS_SteamAchievementRegistry`
- `USOTS_SteamLeaderboardRegistry`

Each definition includes:

- An `InternalId` used in code.
- A `DisplayName` that can be shown in UI.
- An optional `SteamApiName` that the Steam backend expects (falls back to `InternalId`).
- Gameplay tags (via `Tags`) that drive mission result matching.

---

## 4. Hooking Mission Results

At mission completion, construct an `FSOTS_SteamMissionResult` struct and forward it to the subsystems:

```cpp
FSOTS_SteamMissionResult Result;
Result.MissionTag = /* e.g., SAS.Mission.Castle */;
Result.DifficultyTag = /* e.g., SAS.Difficulty.Hard */;
Result.Score = /* final mission score */;
Result.MissionTimeSeconds = /* total time in seconds */;
Result.bNoKills = /* true/false */;
Result.bNoAlerts = /* true/false */;
Result.bPerfectStealth = /* true/false */;
Result.AdditionalTags = /* optional bonus tags */;

AchievementsSubsystem->HandleMissionResult(Result);
LeaderboardsSubsystem->SubmitMissionResultToLeaderboards(Result, PlayerName, /*bOnlyIfBetter=*/true);
```

Any achievement or leaderboard whose definition tags are a subset of `Result.AdditionalTags` (plus the built-in mission/difficulty tags) will be unlocked/updated automatically.
