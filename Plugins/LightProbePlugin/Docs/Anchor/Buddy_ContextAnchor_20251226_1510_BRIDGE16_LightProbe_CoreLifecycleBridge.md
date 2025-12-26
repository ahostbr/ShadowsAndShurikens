Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1510 | Plugin: LightProbePlugin | Pass/Topic: BRIDGE16_CoreLifecycleBridge | Owner: Buddy
Scope: Optional SOTS_Core lifecycle bridge for LightProbePlugin (state-only delegates, OFF by default)

DONE
- Added `ULightProbeCoreBridgeSettings` with `bEnableSOTSCoreLifecycleBridge=false` and `bEnableSOTSCoreBridgeVerboseLogs=false`.
- Added `FLightProbe_CoreLifecycleHook : ISOTS_CoreLifecycleListener` registered via `FSOTS_CoreLifecycleListenerHandle` in module Startup/Shutdown.
- Added internal delegate seams in `LightProbePlugin::CoreBridge`:
  - `OnCoreWorldReady_Native()`
  - `OnCorePrimaryPlayerReady_Native()`
  - Optional travel relays: `OnCorePreLoadMap_Native()` and `OnCorePostLoadMap_Native()` (only bind when `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()` is true).
- Added `SOTS_Core` module + plugin dependency.

VERIFIED
- UNVERIFIED (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Assumed header-only delegate access is undesirable across modules; implemented single-instance delegates via a `.cpp` for stability.

FILES TOUCHED
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
- Plugins/LightProbePlugin/Docs/Worklogs/BuddyWorklog_20251226_151000_BRIDGE16_LightProbe_CoreLifecycleBridge.md

NEXT (Ryan)
- Build the project; confirm LightProbePlugin compiles with SOTS_Core dependency.
- In-editor/PIE: toggle `LightProbe - Core Bridge` setting and confirm:
  - OFF: no behavior change
  - ON: no crash; verbose logs only when verbose enabled; travel relays fire only if Core travel bridge is bound.

ROLLBACK
- Revert the touched files listed above.
