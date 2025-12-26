# Buddy Worklog — 2025-12-26 13:35 — UE5.7 WorldBeginPlay bridge + ModularFeatures

## Goal
Unblock UE 5.7 compilation for `SOTS_Core` by removing/adjusting APIs that changed in 5.7 (World begin-play delegate + Modular Features overloads) and fixing `TObjectPtr` logging.

## What changed
- Replaced usage of `FWorldDelegates::OnWorldBeginPlay` (missing in UE 5.7) with per-world begin-play handlers via `UWorld::AddOnWorldBeginPlayHandler(...)` registered in `HandlePostWorldInitialization`.
- Ensured per-world handlers are removed on world cleanup and subsystem deinit.
- Updated `IModularFeatures::GetModularFeatureImplementations` call sites from the 2-arg “fill array” form to the UE 5.7 1-arg overload returning an array.
- Fixed `GetNameSafe` usage for `TObjectPtr<>` fields by calling `.Get()`.

## Files changed
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Save/SOTS_CoreSaveParticipantRegistry.cpp

## Notes / Risks / UNKNOWN
- UNVERIFIED: `UWorld::AddOnWorldBeginPlayHandler` / `RemoveOnWorldBeginPlayHandler` API availability in the project’s UE 5.7 headers (this was chosen as a likely-stable replacement for the removed global delegate).
- Behavior change risk: begin-play notifications are now bound per-world during post-world init rather than via a global world delegate.

## Verification
- UNVERIFIED: No build/run performed here (Ryan to compile/launch editor).

## Next steps (Ryan)
- Rebuild `SOTS_Core` on UE 5.7.1.
- If compile still fails: capture the next error block from `Saved/Logs/ShadowsAndShurikens.log` and we’ll patch the next API drift.
