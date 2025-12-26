# Buddy Worklog — 2025-12-26 14:58 — BRIDGE15 Steam Core Lifecycle Bridge

## Goal
Implement BRIDGE15 for `SOTS_Steam`: an OFF-by-default `SOTS_Core` lifecycle listener that can (when explicitly allowed) trigger a deterministic and safe Steam availability refresh.

## What changed
- Added `USOTS_SteamCoreBridgeSettings` with a **double-gate**:
  - `bEnableSOTSCoreLifecycleBridge` (master)
  - `bAllowCoreTriggeredSteamInit` (explicit allow)
  - plus `bEnableSOTSCoreBridgeVerboseLogs`
- Added `FSOTS_SteamCoreLifecycleHook : ISOTS_CoreLifecycleListener` registered via `FSOTS_CoreLifecycleListenerHandle`.
- Implemented a minimal, safe init seam by exposing `USOTS_SteamSubsystemBase::ForceRefreshSteamAvailability()` which calls the existing internal refresh.
- Hook calls `ForceRefreshSteamAvailability()` on Steam subsystems (Achievements / Leaderboards / MissionResultBridge) once per `UGameInstance`.
- Wired module dependencies and plugin dependency on `SOTS_Core`.

## Files changed
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

## Notes / Risks / Unknowns
- UNVERIFIED: No UE build/run performed in this pass.
- The new `ForceRefreshSteamAvailability()` method is additive and should be safe, but needs compile verification.
- Behavior change is guarded by two settings gates; default remains inert.

## Verification status
- UNVERIFIED: not built, not run.

## Follow-ups / Next steps (Ryan)
- Build and run PIE with gates OFF/ON as described in the docs.
- Confirm no duplicate listener registration and no unexpected logs when disabled.
