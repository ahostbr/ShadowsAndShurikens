Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1335 | Plugin: SOTS_Core | Pass/Topic: UE57_WorldBeginPlayBridge | Owner: Buddy
Scope: UE 5.7 compile unblock (world begin-play delegate + modular features overload + TObjectPtr logging)

DONE
- Swapped `FWorldDelegates::OnWorldBeginPlay` binding for per-world `UWorld::AddOnWorldBeginPlayHandler` registered during `HandlePostWorldInitialization`.
- Tracked begin-play delegate handles per-world and removed them during world cleanup + subsystem deinit.
- Updated `IModularFeatures::GetModularFeatureImplementations` usage to the UE 5.7 1-arg overload returning an array.
- Fixed `GetNameSafe` calls on `FSOTS_CoreLifecycleSnapshot` `TObjectPtr<>` fields by using `.Get()`.

VERIFIED
- UNVERIFIED (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Assumed `UWorld::AddOnWorldBeginPlayHandler` / `RemoveOnWorldBeginPlayHandler` exists in the project’s UE 5.7 headers.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Save/SOTS_CoreSaveParticipantRegistry.cpp

NEXT (Ryan)
- Build the project in UE 5.7.1; confirm `SOTS_Core` compiles.
- Validate that lifecycle events still fire (WorldStartPlay) and don’t double-fire with map travel settings.

ROLLBACK
- Revert the touched files above.
