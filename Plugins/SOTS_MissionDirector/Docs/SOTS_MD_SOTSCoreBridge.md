# SOTS_MissionDirector ↔ SOTS_Core Bridge (BRIDGE9)

## Goal
Provide an OPTIONAL, disabled-by-default integration so MissionDirector receives deterministic lifecycle/map-travel callbacks via SOTS_Core.

This bridge is **state-only**: it caches pointers and/or updates internal “core lifecycle seen” state. It does **not** start missions, request travel, activate objectives, or trigger saving.

## How it works
When enabled, SOTS_MissionDirector registers a small `ISOTS_CoreLifecycleListener` implementation with the ModularFeatures system under `SOTS.CoreLifecycleListener`.

Callbacks used:
- `OnSOTS_WorldStartPlay(UWorld*)` → calls `USOTS_MissionDirectorSubsystem::HandleCoreWorldStartPlay(World)`.
- `OnSOTS_PawnPossessed(APlayerController*, APawn*)` → calls `USOTS_MissionDirectorSubsystem::HandleCorePrimaryPlayerReady(PC, Pawn)`.

Optional travel signals (only when Core’s map-travel bridge is enabled):
- Binds to `USOTS_CoreLifecycleSubsystem::OnPreLoadMap_Native` and `OnPostLoadMap_Native`.
- Calls `USOTS_MissionDirectorSubsystem::HandleCorePreLoadMap(MapName)` and `HandleCorePostLoadMap(World)`.

## Enablement
1) In **SOTS_Core** settings:
- Enable world lifecycle dispatch (Core world delegate bridge): `bEnableWorldDelegateBridge`.
- (Optional) Enable travel callbacks: `bEnableMapTravelBridge`.

2) In **SOTS_MissionDirector** settings:
- Enable `USOTS_MissionDirectorCoreBridgeSettings::bEnableSOTSCoreLifecycleBridge`.
- Optional logging: `bEnableSOTSCoreBridgeVerboseLogs`.

## Notes / safety
- Defaults are OFF.
- No Blueprint edits are required.
- Registration is done at module startup; settings are not hot-reloaded during runtime.

## Rollback
- Disable `bEnableSOTSCoreLifecycleBridge` and (if desired) disable Core’s delegate/map-travel bridges.
- No assets need reparenting; no data migrations.
