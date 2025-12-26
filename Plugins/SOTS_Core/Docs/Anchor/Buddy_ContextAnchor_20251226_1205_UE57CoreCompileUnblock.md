Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1205 | Plugin: SOTS_Core | Pass/Topic: UE57_CoreCompileUnblock | Owner: Buddy
Scope: UE 5.7 compile fixes in SOTS_Core (unity duplicate + modular-features API)

DONE
- Centralized `GetLifecycleSubsystem` helper into a shared inline header to prevent unity build duplicate definition errors.
- Removed uses of `IModularFeatures::IsAvailable()` (not present in UE 5.7 per compiler errors) and relied on `IModularFeatures::Get()`.
- Updated lifecycle listener interface to derive from `IModularFeature` so registration uses `IModularFeature*` as required.

VERIFIED
- 

UNVERIFIED / ASSUMPTIONS
- No build executed after this change set.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystemUtil.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_GameModeBase.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_HUDBase.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_PlayerControllerBase.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Lifecycle/SOTS_CoreLifecycleListenerHandle.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Save/SOTS_CoreSaveParticipantRegistry.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Tests/SOTS_CoreLifecycleTests.cpp

NEXT (Ryan)
- Build in UE 5.7.1; confirm C2084/C2039/C2664 errors are cleared.

ROLLBACK
- Revert the above files or revert the associated commit.
