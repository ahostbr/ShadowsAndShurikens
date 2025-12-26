Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1501 | Plugin: SOTS_GAS_Plugin | Pass/Topic: BRIDGE18_GAS_CoreBridge | Owner: Buddy
Scope: Optional SOTS_Core lifecycle + save-participant bridge for SOTS_GAS_Plugin (default OFF)

DONE
- Added `USOTS_GASCoreBridgeSettings` (default OFF): `bEnableSOTSCoreLifecycleBridge`, `bEnableSOTSCoreBridgeVerboseLogs`, `bEnableSOTSCoreSaveParticipantBridge`.
- Added `FGAS_CoreLifecycleHook : ISOTS_CoreLifecycleListener` registered via `FSOTS_CoreLifecycleListenerHandle` when enabled.
- Added `SOTS_GAS::CoreBridge` native delegates: `OnCoreWorldReady_Native`, `OnCorePrimaryPlayerReady_Native`, `OnCorePreLoadMap_Native`, `OnCorePostLoadMap_Native`.
- Added `FGAS_SaveParticipant : ISOTS_CoreSaveParticipant` (id `GAS`) with fragment `GAS.State` (opaque bytes) serialized from `FSOTS_AbilityProfileData` and applied via `USOTS_AbilitySubsystem::ApplyProfileData`.
- Wired dependencies: `SOTS_GAS_Plugin.Build.cs` adds `DeveloperSettings` + `SOTS_Core`; `SOTS_GAS_Plugin.uplugin` adds `SOTS_Core` plugin dependency.

VERIFIED
- (none) No build/editor/runtime verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes `FSOTS_AbilityProfileData::StaticStruct()->SerializeItem` is stable/acceptable for opaque payload bytes.
- Assumes resolving `USOTS_AbilitySubsystem` via `GEngine->GetWorldContexts()` is sufficient in practice.

FILES TOUCHED
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/SOTS_GAS_Plugin.Build.cs
- Plugins/SOTS_GAS_Plugin/SOTS_GAS_Plugin.uplugin
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_Plugin.cpp
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/SOTS_GASCoreBridgeSettings.h
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GASCoreBridgeSettings.cpp
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Public/SOTS_GASCoreBridgeDelegates.h
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GASCoreBridgeDelegates.cpp
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GASCoreLifecycleHook.h
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GASCoreLifecycleHook.cpp
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GASCoreSaveParticipant.h
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GASCoreSaveParticipant.cpp
- Plugins/SOTS_GAS_Plugin/Docs/SOTS_GAS_SOTSCoreBridge.md

NEXT (Ryan)
- Build the project and confirm `SOTS_GAS_Plugin` loads with bridge settings OFF.
- Enable lifecycle bridge and confirm delegate broadcasts on WorldStartPlay and local PawnPossessed.
- Confirm travel delegates only broadcast when `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()` is true.
- Enable save participant bridge and validate `GAS.State` fragment round-trips ability profile state across save/load.

ROLLBACK
- Revert the touched files listed above.