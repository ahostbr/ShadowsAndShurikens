Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1201 | Plugin: SOTS_MissionDirector | Pass/Topic: BRIDGE9_CoreLifecycleBridge | Owner: Buddy
Scope: Optional, disabled-by-default MissionDirector integration with SOTS_Core lifecycle + travel signals (state-only)

DONE
- Added `USOTS_MissionDirectorCoreBridgeSettings` with `bEnableSOTSCoreLifecycleBridge` + `bEnableSOTSCoreBridgeVerboseLogs` (defaults OFF).
- Added `FMissionDirector_CoreLifecycleHook : ISOTS_CoreLifecycleListener` and registered it in `FSOTS_MissionDirectorModule::StartupModule` when enabled.
- Forwarded Core lifecycle events to `USOTS_MissionDirectorSubsystem` state-only handlers.
- If Core `bEnableMapTravelBridge` is enabled, bound `USOTS_CoreLifecycleSubsystem::OnPreLoadMap_Native` / `OnPostLoadMap_Native` and forwarded to state-only handlers.
- Declared `SOTS_Core` dependency in MissionDirector Build.cs and .uplugin.

VERIFIED
- No Unreal build/run performed.

UNVERIFIED / ASSUMPTIONS
- Assumes Core lifecycle dispatch is enabled and emitting events (Core setting `bEnableWorldDelegateBridge`).
- Assumes travel delegates fire only when Core `bEnableMapTravelBridge` is enabled.

FILES TOUCHED
- Plugins/SOTS_MissionDirector/SOTS_MissionDirector.uplugin
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/SOTS_MissionDirector.Build.cs
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorCoreBridgeSettings.h
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorCoreBridgeSettings.cpp
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorCoreLifecycleHook.h
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorCoreLifecycleHook.cpp
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorModule.cpp
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp
- Plugins/SOTS_MissionDirector/Docs/SOTS_MD_SOTSCoreBridge.md

NEXT (Ryan)
- Enable Core `bEnableWorldDelegateBridge` and MissionDirector `bEnableSOTSCoreLifecycleBridge`, then run `SOTS.Core.BridgeHealth` and confirm listener registration.
- Optionally enable Core `bEnableMapTravelBridge` and confirm PreLoad/PostLoad callbacks reach MissionDirector (no travel behavior should be triggered by this bridge).

ROLLBACK
- Revert the touched files or disable `bEnableSOTSCoreLifecycleBridge` (and Core delegate bridges if enabled).
