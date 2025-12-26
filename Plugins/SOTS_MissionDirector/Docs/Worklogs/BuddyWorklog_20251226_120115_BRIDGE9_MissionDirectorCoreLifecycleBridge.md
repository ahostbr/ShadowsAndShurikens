# Buddy Worklog — 2025-12-26 — BRIDGE9 MissionDirector Core lifecycle bridge

Goal
- Implement BRIDGE9: optional, disabled-by-default bridge so `SOTS_MissionDirector` receives deterministic lifecycle/map-travel callbacks via `SOTS_Core` (state-only; no travel/saving/objective behavior).

Constraints honored
- Add-only; minimal diffs.
- No Blueprint edits.
- No Unreal build/run.
- Bridge defaults OFF.

Evidence (RepoIntel / RAG-first)
- Core lifecycle listener feature + callbacks:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h` (lines 1-22)
- Core lifecycle subsystem exposes map travel delegates (SPINE4 travel bridge surface):
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h` (lines 55-170)
  - Core binds map-travel engine delegates only when enabled:
    - `Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp` (lines 10-90)
- MissionDirector is a `UGameInstanceSubsystem` and is the natural recipient for state-only lifecycle caching:
  - `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h` (lines 25-120)

What changed
- Added `SOTS_Core` dependency + optional listener registration to `SOTS_MissionDirector`.
- Added `USOTS_MissionDirectorCoreBridgeSettings` (defaults OFF).
- Added `FMissionDirector_CoreLifecycleHook : ISOTS_CoreLifecycleListener`:
  - For WorldStartPlay + PawnPossessed: forwards to new state-only MissionDirector subsystem handlers.
  - If `USOTS_CoreSettings::bEnableMapTravelBridge` is enabled: binds to CoreLifecycleSubsystem’s `OnPreLoadMap_Native`/`OnPostLoadMap_Native` and forwards to state-only handlers.
- Added state-only handlers on `USOTS_MissionDirectorSubsystem`:
  - `HandleCoreWorldStartPlay`, `HandleCorePrimaryPlayerReady`, `HandleCorePreLoadMap`, `HandleCorePostLoadMap`.

Files changed/created
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/SOTS_MissionDirector.Build.cs`
  - Add `SOTS_Core` private dependency.
- `Plugins/SOTS_MissionDirector/SOTS_MissionDirector.uplugin`
  - Add plugin dependency on `SOTS_Core`.
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorCoreBridgeSettings.h`
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorCoreBridgeSettings.cpp`
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorCoreLifecycleHook.h`
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorCoreLifecycleHook.cpp`
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorModule.cpp`
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h`
- `Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp`
- `Plugins/SOTS_MissionDirector/Docs/SOTS_MD_SOTSCoreBridge.md`

Verification status
- UNVERIFIED: No build/run performed (per constraints).
- Static analysis: VS Code reported no errors in touched files.

Risks / unknowns
- Travel callbacks are routed via `USOTS_CoreLifecycleSubsystem` delegates (not via `ISOTS_CoreLifecycleListener`, which has no travel methods). This is intentional to avoid changing Core’s public listener interface.

Next (Ryan)
- Enable Core settings (`bEnableWorldDelegateBridge`, optionally `bEnableMapTravelBridge`) and MissionDirector setting (`bEnableSOTSCoreLifecycleBridge`).
- Run `SOTS.Core.BridgeHealth` and confirm the MissionDirector listener is listed.
- Verify travel delegates fire only when `bEnableMapTravelBridge` is enabled.

Cleanup
- Pending in this worklog: delete `Plugins/SOTS_MissionDirector/Binaries` and `Plugins/SOTS_MissionDirector/Intermediate`.
