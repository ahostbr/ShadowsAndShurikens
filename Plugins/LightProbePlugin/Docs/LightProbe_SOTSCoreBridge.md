# LightProbePlugin ↔ SOTS_Core Lifecycle Bridge (BRIDGE16)

## Goal
Add an **OFF-by-default** bridge so LightProbePlugin can receive deterministic lifecycle and (optionally) travel events via SOTS_Core.

This pass is **state-only**:
- Does not change how `ULightLevelProbeComponent` samples light.
- Does not change GlobalStealth ownership.
- Does not add tick/polling.

## Settings
`ULightProbeCoreBridgeSettings` (Project Settings)
- `bEnableSOTSCoreLifecycleBridge` (default **false**)
- `bEnableSOTSCoreBridgeVerboseLogs` (default **false**)

## Events surfaced (native delegates)
These are internal C++ hook seams (no automatic gameplay wiring in this pass):
- `LightProbePlugin::CoreBridge::OnCoreWorldReady_Native()` — broadcast on `OnSOTS_WorldStartPlay`.
- `LightProbePlugin::CoreBridge::OnCorePrimaryPlayerReady_Native()` — broadcast on `OnSOTS_PawnPossessed` (local controller only).

## Optional travel events
If SOTS_Core’s map travel bridge is bound (`USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()`), the bridge also listens to:
- `OnPreLoadMap_Native` → broadcasts `OnCorePreLoadMap_Native` and clears cached PC/Pawn.
- `OnPostLoadMap_Native` → broadcasts `OnCorePostLoadMap_Native`.

## Implementation summary
- Listener: `FLightProbe_CoreLifecycleHook : ISOTS_CoreLifecycleListener`
- Registration: `FSOTS_CoreLifecycleListenerHandle` in LightProbePlugin module startup/shutdown

## Verification
UNVERIFIED (no build/run in this pass). Ryan should verify:
- With bridge OFF: no behavior change.
- With bridge ON: delegate broadcasts occur; no duplicate spam; no crashes.
