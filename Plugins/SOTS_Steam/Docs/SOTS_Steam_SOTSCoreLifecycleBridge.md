# SOTS_Steam ↔ SOTS_Core Lifecycle Bridge (BRIDGE15)

## Goal
Provide an **OFF-by-default** bridge from SOTS_Core lifecycle events into SOTS_Steam, allowing a deterministic and safe refresh of Steam OnlineSubsystem availability at runtime.

This bridge is designed to be **state-only**:
- It does **not** create UI.
- It does **not** perform IO.
- It does **not** push achievements / leaderboards / stats.
- It only refreshes cached OnlineSubsystem availability used by SOTS_Steam subsystems.

## Why this exists
SOTS_Steam subsystems perform a safe availability check during subsystem initialization, but depending on engine/system timing the OnlineSubsystem may not be ready at that exact moment.

When explicitly allowed, this bridge re-checks availability on deterministic lifecycle moments:
- `OnSOTS_WorldStartPlay`
- `OnSOTS_PlayerControllerBeginPlay`

## Settings
`USOTS_SteamCoreBridgeSettings` (Project Settings)
- `bEnableSOTSCoreLifecycleBridge` (default **false**) — master gate
- `bAllowCoreTriggeredSteamInit` (default **false**) — explicit second gate (required)
- `bEnableSOTSCoreBridgeVerboseLogs` (default **false**) — verbose logging

Both gates must be enabled for any runtime work to occur.

## Implementation notes
- Listener: `FSOTS_SteamCoreLifecycleHook : ISOTS_CoreLifecycleListener`
- Registration: `FSOTS_CoreLifecycleListenerHandle` in module `StartupModule` / `ShutdownModule`
- Safe init seam:
  - `USOTS_SteamSubsystemBase::ForceRefreshSteamAvailability()`
  - Called on these subsystems (if present on the `UGameInstance`):
    - `USOTS_SteamAchievementsSubsystem`
    - `USOTS_SteamLeaderboardsSubsystem`
    - `USOTS_SteamMissionResultBridgeSubsystem`
- Duplicate suppression: only one attempt per `UGameInstance`.

## Verification
UNVERIFIED (no build/run in this pass). Ryan should verify:
- With both gates OFF: no behavior change.
- With bridge ON but allow OFF: no behavior change.
- With both ON: availability refresh runs once and does not crash; Steam-required warning still only occurs when Steam is required and unavailable.
