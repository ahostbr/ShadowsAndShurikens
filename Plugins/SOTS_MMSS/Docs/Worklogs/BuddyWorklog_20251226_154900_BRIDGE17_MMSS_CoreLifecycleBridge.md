# Buddy Worklog — 2025-12-26 — BRIDGE17 — SOTS_MMSS ↔ SOTS_Core lifecycle/travel bridge

## Goal
Implement `[SOTS_VSCODEBUDDY_CODEX_MAX_SOTS_CORE_BRIDGE17]`:
- Add an OPTIONAL, disabled-by-default bridge so SOTS_MMSS can receive deterministic lifecycle callbacks via SOTS_Core.
- State-only in this pass (cache/broadcast only; no music behavior changes).
- Map travel events are only relayed if Core travel bridge is already bound.

## Evidence-first notes (paths)
### MMSS seams
- `USOTS_MMSSSubsystem::Initialize/Deinitialize` exists and manages MMSS state + Profile provider registration:
  - `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSSubsystem.cpp` (Initialize/Deinitialize; profile provider register/unregister)

### Core travel delegates (SPINE4)
- Core exposes travel delegates and a runtime gate:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h` (`IsMapTravelBridgeBound`, `OnPreLoadMap_Native`, `OnPostLoadMap_Native`)
  - `Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp` (`HandlePreLoadMap`, `HandlePostLoadMapWithWorld`, `NotifyPreLoadMap`, `NotifyPostLoadMap`, `IsMapTravelBridgeBound`)

## What changed
- Added MMSS DeveloperSettings toggles (default OFF).
- Added state-only native delegate surface for WorldReady / PrimaryPlayerReady / PreLoadMap / PostLoadMap.
- Implemented lifecycle listener that:
  - Broadcasts `OnCoreWorldReady_Native` on `OnSOTS_WorldStartPlay`.
  - Broadcasts `OnCorePrimaryPlayerReady_Native` on `OnSOTS_PawnPossessed` (local controller only; dup-suppressed).
  - Binds and relays PreLoad/PostLoad map only if `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()`.
- Registered/unregistered the listener from the MMSS module Startup/Shutdown, but only registers at startup if the setting is enabled (no hot reload).

## Files changed/added
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/SOTS_MMSS.Build.cs`
  - Add private deps: `DeveloperSettings`, `SOTS_Core`.
- `Plugins/SOTS_MMSS/SOTS_MMSS.uplugin`
  - Declare plugin dependency on `SOTS_Core`.
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSModule.cpp`
  - Register/unregister the Core lifecycle bridge on module startup/shutdown.
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSSCoreBridgeSettings.h`
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreBridgeSettings.cpp`
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSSCoreBridgeDelegates.h`
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreBridgeDelegates.cpp`
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreLifecycleHook.h`
- `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreLifecycleHook.cpp`
- `Plugins/SOTS_MMSS/Docs/SOTS_MMSS_SOTSCoreBridge.md`

## Defaults / behavior impact
- Defaults are OFF (`bEnableSOTSCoreLifecycleBridge=false`).
- No music behavior changes in this pass (only broadcasts delegates when enabled).

## Verification
- UNVERIFIED: no Unreal build/run performed.
- Static analysis: no errors reported in touched C++ files.

## Cleanup
- Per SOTS law: remove `Plugins/SOTS_MMSS/Binaries` and `Plugins/SOTS_MMSS/Intermediate` after edits (performed after implementation).

## Follow-ups (Ryan)
- Enable setting and confirm logs/delegates fire in expected order.
- Confirm travel relays only bind when Core travel bridge is active.
