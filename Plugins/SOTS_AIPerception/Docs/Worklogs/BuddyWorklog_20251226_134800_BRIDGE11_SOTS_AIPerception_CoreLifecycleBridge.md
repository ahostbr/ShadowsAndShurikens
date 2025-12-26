# Buddy Worklog — 2025-12-26 13:48 — SOTS_AIPerception BRIDGE11 (Core lifecycle bridge, state-only)

## Goal
Implement BRIDGE11: add an **OFF-by-default** bridge so SOTS_AIPerception can receive deterministic lifecycle callbacks from SOTS_Core (WorldStartPlay, PawnPossessed; optional travel events via Core lifecycle subsystem when Core map travel bridge is enabled).

Constraints honored:
- Add-only, minimal diffs.
- No Blueprint edits.
- No Unreal build/run.
- Default behavior inert (bridge OFF; no AI behavior changes).

## Evidence (RepoIntel)
- Core lifecycle listener interface: `ISOTS_CoreLifecycleListener` in `Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h`.
- AIPerception module entrypoint: `FSOTS_AIPerceptionModule` in `Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerception.cpp`.
- AIPerception owns a `UWorldSubsystem`: `USOTS_AIPerceptionSubsystem`.

## What changed
- Added `SOTS_Core` as a dependency for SOTS_AIPerception (Build.cs + .uplugin plugin dependency).
- Added AIPerception bridge settings (default OFF):
  - `USOTS_AIPerceptionCoreBridgeSettings::bEnableSOTSCoreLifecycleBridge`
  - `USOTS_AIPerceptionCoreBridgeSettings::bEnableSOTSCoreBridgeVerboseLogs`
- Implemented `FAIPerception_CoreLifecycleHook : ISOTS_CoreLifecycleListener`:
  - Forwards lifecycle callbacks to `USOTS_AIPerceptionSubsystem` state-only handlers.
  - Optionally binds Core travel delegates (`OnPreLoadMap_Native` / `OnPostLoadMap_Native`) if Core `bEnableMapTravelBridge=true`.
- Added state-only handlers + internal native delegates on `USOTS_AIPerceptionSubsystem`:
  - `HandleCoreWorldReady`, `HandleCorePrimaryPlayerReady`, `HandleCorePreLoadMap`, `HandleCorePostLoadMap`

## Files changed
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/SOTS_AIPerception.Build.cs
- Plugins/SOTS_AIPerception/SOTS_AIPerception.uplugin
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerception.cpp
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionSubsystem.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionSubsystem.cpp

## Files created
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionCoreBridgeSettings.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionCoreBridgeSettings.cpp
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionCoreLifecycleHook.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionCoreLifecycleHook.cpp
- Plugins/SOTS_AIPerception/Docs/SOTS_AIPerception_SOTSCoreBridge.md

## Verification status
- UNVERIFIED: No Unreal build/run performed (Ryan will verify).

## Follow-ups / Next steps (Ryan)
- Build and verify SOTS_AIPerception compiles.
- Enable Core dispatch + enable `bEnableSOTSCoreLifecycleBridge` and confirm no AI behavior changes unless downstream code opts into the new delegates.
