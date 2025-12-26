# Buddy Worklog — 2025-12-26 15:10 — BRIDGE16 LightProbePlugin Core Lifecycle Bridge

## Goal
Implement BRIDGE16: add an optional (OFF-by-default) bridge so LightProbePlugin can receive deterministic lifecycle events via SOTS_Core, without altering probe sampling or stealth behavior.

## Evidence (RepoIntel)
- LightProbePlugin is primarily an `UActorComponent` (`ULightLevelProbeComponent`) that runs on actor `BeginPlay` and periodically samples a render target; it feeds a normalized value into `USOTS_PlayerStealthComponent::SetLightLevel`.
  - Source: `Plugins/LightProbePlugin/Source/LightProbePlugin/Public/LightLevelProbeComponent.h`
  - Implementation: `Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp` (`BeginPlay`, `UpdateLightLevel`)
- SOTS_Core provides travel delegates on `USOTS_CoreLifecycleSubsystem` (`OnPreLoadMap_Native`, `OnPostLoadMap_Native`) and a capability query `IsMapTravelBridgeBound()`.
  - Source: `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h`

## What changed
- Added `ULightProbeCoreBridgeSettings` with `bEnableSOTSCoreLifecycleBridge=false` and `bEnableSOTSCoreBridgeVerboseLogs=false`.
- Added native delegate hook seams (single-instance per module):
  - `OnCoreWorldReady_Native`
  - `OnCorePrimaryPlayerReady_Native`
  - plus optional travel delegate relays `OnCorePreLoadMap_Native` / `OnCorePostLoadMap_Native`
- Added `FLightProbe_CoreLifecycleHook : ISOTS_CoreLifecycleListener`:
  - `OnSOTS_WorldStartPlay` broadcasts world-ready
  - `OnSOTS_PawnPossessed` broadcasts primary-player-ready (local controller only) with duplicate suppression
  - If Core travel bridge is bound, binds to `USOTS_CoreLifecycleSubsystem` map travel delegates and forwards them state-only
- Wired module startup/shutdown registration using `FSOTS_CoreLifecycleListenerHandle`.
- Added `SOTS_Core` module + plugin dependency.

## Files changed
- Plugins/LightProbePlugin/LightProbePlugin.uplugin
- Plugins/LightProbePlugin/Source/LightProbePlugin/LightProbePlugin.Build.cs
- Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightProbePluginModule.cpp
- Plugins/LightProbePlugin/Source/LightProbePlugin/Public/LightProbeCoreBridgeSettings.h
- Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightProbeCoreBridgeSettings.cpp
- Plugins/LightProbePlugin/Source/LightProbePlugin/Public/LightProbeCoreBridgeDelegates.h
- Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightProbeCoreBridgeDelegates.cpp
- Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightProbeCoreLifecycleHook.h
- Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightProbeCoreLifecycleHook.cpp
- Plugins/LightProbePlugin/Docs/LightProbe_SOTSCoreBridge.md

## Notes / Risks / Unknowns
- UNVERIFIED: no UE build/run.
- The bridge does not automatically wire any LightProbe behavior; it only broadcasts delegates when enabled.

## Verification status
- UNVERIFIED.

## Follow-ups (Ryan)
- Build and run PIE:
  - bridge OFF: verify LightProbe behavior unchanged.
  - bridge ON: verify verbose logs (if enabled) and delegate broadcasts (if bound by any consumer).
