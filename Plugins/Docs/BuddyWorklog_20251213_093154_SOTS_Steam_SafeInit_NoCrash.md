# Prompt ID
- Not provided.

# Goal
- Make SOTS_Steam safe when Steam/OSS is absent or non-Steam while still working when Steam is available.

# What changed
- Added configurable Steam gating (enable/require/preferred subsystem) to `USOTS_SteamSettings`.
- Hardened `USOTS_SteamSubsystemBase` to resolve OnlineSubsystem safely, cache availability, and expose `IsSteamAvailable()` for Blueprint callers.
- Routed achievement and leaderboard online usage through the new availability gate and safe subsystem resolver.

# Files changed
- Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamSettings.h
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamSettings.cpp
- Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamSubsystemBase.h
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamSubsystemBase.cpp
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamAchievementsSubsystem.cpp
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamLeaderboardsSubsystem.cpp

# Notes / Decisions
- No hard module loads; OnlineSubsystem is fetched via `IOnlineSubsystem::Get` with preferred name fallback.
- If `bRequireSteamSubsystem` is true and a non-Steam subsystem is active, a single warning is logged and Steam features are disabled; otherwise non-Steam runs stay silent.
- Online calls now early-out when Steam is unavailable, avoiding null interface usage in editor or non-Steam runs.

# Verification
- Not run (no builds or automated tests).

# Cleanup confirmation
- Deleted Plugins/SOTS_Steam/Binaries and Plugins/SOTS_Steam/Intermediate; no build triggered.

# Follow-ups
- Consider surfacing availability state in UI/tools and updating default configs for non-Steam editor sessions as needed.
