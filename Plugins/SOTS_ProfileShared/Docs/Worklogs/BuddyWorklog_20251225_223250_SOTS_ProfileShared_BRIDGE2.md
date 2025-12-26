# Buddy Worklog - SOTS_ProfileShared BRIDGE2 - 2025-12-25 22:32:50

## Prompt
SOTS_Core BRIDGE2 - SOTS_ProfileShared lifecycle listener (Core bridge, defaults OFF).

## Goal
Add an optional, disabled-by-default Core lifecycle bridge so ProfileShared can receive deterministic world and player-ready events and expose state-only delegates without changing save or travel behavior.

## RepoIntel / evidence
- SOTS_Core lifecycle listener interface and feature name:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h:14`
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h:16`
- ProfileShared snapshot seams + existing delegate:
  - `USOTS_ProfileSubsystem::BuildSnapshotFromWorld` - `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp:195`
  - `USOTS_ProfileSubsystem::ApplySnapshotToWorld` - `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp:215`
  - `OnProfileRestored` - `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h:62`

## What changed
- Added ProfileShared Core bridge settings (default OFF) for lifecycle bridge + verbose logs.
- Registered a Core lifecycle listener in the ProfileShared module:
  - WorldStartPlay -> caches world and fires `OnCoreWorldStartPlay` (state-only).
  - PostLogin -> marks player session started (state-only).
  - PawnPossessed -> updates primary player ready state and fires `OnCorePrimaryPlayerReady`.
- Added state-only delegates and handlers in `USOTS_ProfileSubsystem`.
- Declared SOTS_Core and DeveloperSettings dependencies for the module.
- Documented enablement and behavior in a new bridge doc.

## Files touched
- `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/SOTS_ProfileShared.Build.cs`
- `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSharedCoreBridgeSettings.h`
- `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSharedCoreBridgeSettings.cpp`
- `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h`
- `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp`
- `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSharedModule.cpp`
- `Plugins/SOTS_ProfileShared/Docs/SOTS_ProfileShared_SOTSCoreBridge.md`
- `Plugins/SOTS_ProfileShared/Docs/Worklogs/BuddyWorklog_20251225_223250_SOTS_ProfileShared_BRIDGE2.md`

## Notes / decisions
- Listener is registered at module startup; callbacks are inert unless `bEnableSOTSCoreLifecycleBridge` is true.
- State-only delegates are native (no BP binding) to keep the bridge minimal and safe.
- Duplicate suppression is handled both in the listener and subsystem state caches.

## Verification
- Not run (per instructions).

## Cleanup
- Attempted to delete `Plugins/SOTS_ProfileShared/Binaries` and `Plugins/SOTS_ProfileShared/Intermediate`; removal failed due to locked files (likely `UnrealEditor-SOTS_ProfileShared.*`).

## Follow-ups
- Enable Core dispatch + ProfileShared bridge in a controlled test map and confirm Core diagnostics.
