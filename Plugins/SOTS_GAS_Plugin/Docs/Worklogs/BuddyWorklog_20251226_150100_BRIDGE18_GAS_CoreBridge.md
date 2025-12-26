# Buddy Worklog — 2025-12-26 — BRIDGE18 — SOTS_GAS_Plugin ↔ SOTS_Core

## goal
- Implement [BRIDGE18] optional `SOTS_GAS_Plugin` → `SOTS_Core` bridge.
- Add lifecycle bridge (WorldStartPlay + PawnPossessed; travel relays only if Core travel bridge is bound).
- Add Save Contract participant ONLY because existing persistence seams are present (profile snapshot + serialize/apply helpers).

## what changed
- Added `USOTS_GASCoreBridgeSettings` (default OFF) controlling:
  - lifecycle listener registration
  - verbose bridge logs
  - save participant registration
- Added `FGAS_CoreLifecycleHook : ISOTS_CoreLifecycleListener` (state-only) broadcasting `SOTS_GAS::CoreBridge` delegates.
  - Travel relays (PreLoadMap/PostLoadMap) are bound only when `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()` is true.
- Added `FGAS_SaveParticipant : ISOTS_CoreSaveParticipant` (optional, default OFF)
  - Builds fragment `GAS.State` as opaque bytes serialized from `FSOTS_AbilityProfileData`.
  - Applies fragment through `USOTS_AbilitySubsystem::ApplyProfileData`.
- Updated `SOTS_GAS_Plugin` module startup/shutdown to register the lifecycle listener + save participant only when enabled.
- Added required dependency wiring:
  - `SOTS_GAS_Plugin.Build.cs`: added `DeveloperSettings` + `SOTS_Core` to private deps.
  - `SOTS_GAS_Plugin.uplugin`: added `SOTS_Core` plugin dependency.

## Save Contract decision
- Save Contract fragment implemented? **YES**.
- Evidence: `USOTS_AbilitySubsystem` owns profile-persistent state and already has an explicit snapshot seam (`BuildProfileData` / `ApplyProfileData`) using `FSOTS_AbilityProfileData`.
- Implementation uses USTRUCT serialization as opaque bytes (no invented formats).

## files changed
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

## notes + risks / unknowns
- The save participant resolves `USOTS_AbilitySubsystem` by scanning `GEngine->GetWorldContexts()` (pattern used by other participants). If multiple game instances/worlds exist, this may pick the first with the subsystem.
- No behavior changes unless the settings are enabled.

## verification status
- UNVERIFIED: No Unreal build/run performed (per SOTS law).
- Static checks: pending in this pass (run after cleanup).

## next steps (Ryan)
- Enable `SOTS GAS - Core Bridge` settings and confirm:
  - lifecycle delegates fire (WorldStartPlay + local PawnPossessed)
  - travel relays only bind when Core travel bridge is bound
  - save participant appears in Core save participant enumeration and correctly round-trips ability profile state
