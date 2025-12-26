Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1458 | Plugin: SOTS_Steam | Pass/Topic: BRIDGE15_CoreLifecycleBridge | Owner: Buddy
Scope: Optional SOTS_Core lifecycle listener that can refresh Steam availability (double-gated, OFF by default)

DONE
- Added `USOTS_SteamCoreBridgeSettings` with `bEnableSOTSCoreLifecycleBridge=false`, `bAllowCoreTriggeredSteamInit=false`, `bEnableSOTSCoreBridgeVerboseLogs=false`.
- Added `FSOTS_SteamCoreLifecycleHook : ISOTS_CoreLifecycleListener` and registration via `FSOTS_CoreLifecycleListenerHandle` in `SOTS_SteamModule`.
- Added `USOTS_SteamSubsystemBase::ForceRefreshSteamAvailability()` and invoked it (once per `UGameInstance`) for Achievements/Leaderboards/MissionResult subsystems on `OnSOTS_WorldStartPlay` and `OnSOTS_PlayerControllerBeginPlay`.
- Added `SOTS_Core` module + plugin dependency.

VERIFIED
- UNVERIFIED (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Assumed exposing `ForceRefreshSteamAvailability()` is acceptable as the minimal safe-init seam (it only wraps existing refresh logic).

FILES TOUCHED
- Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamSubsystemBase.h
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamSubsystemBase.cpp
- Plugins/SOTS_Steam/Source/SOTS_Steam/Public/SOTS_SteamCoreBridgeSettings.h
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamCoreBridgeSettings.cpp
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamCoreLifecycleHook.h
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamCoreLifecycleHook.cpp
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/SOTS_SteamModule.cpp
- Plugins/SOTS_Steam/Source/SOTS_Steam/SOTS_Steam.Build.cs
- Plugins/SOTS_Steam/SOTS_Steam.uplugin
- Plugins/SOTS_Steam/Docs/SOTS_Steam_SOTSCoreLifecycleBridge.md

NEXT (Ryan)
- Build the project and ensure `SOTS_Steam` compiles with the added `SOTS_Core` dependency.
- In-editor: toggle `SOTS Steam - Core Bridge` settings and confirm:
  - both OFF: no behavior change
  - bridge ON but allow OFF: no behavior change
  - both ON: one-time refresh logs (if verbose) and no crashes.

ROLLBACK
- Revert the touched files listed above.
