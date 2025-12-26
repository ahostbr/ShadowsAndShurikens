# SOTS_Debug ↔ SOTS_Core Bridge (BRIDGE20)

## Goal
Provide an **opt-in**, **disabled-by-default** integration so SOTS_Debug can receive deterministic SOTS_Core lifecycle callbacks, improving debug readiness/reset without relying on timing.

## Non-goals
- No Blueprint edits.
- No UE build/run verification by Buddy.
- No new debug widgets or tick.

## Settings
Project Settings → **SOTS Debug - Core Bridge**

Class: `USOTS_DebugCoreBridgeSettings`
- `bEnableSOTSCoreLifecycleBridge` (default **false**)
- `bEnableSOTSCoreBridgeVerboseLogs` (default **false**)

Config example:
```ini
[/Script/SOTS_Debug.SOTS_DebugCoreBridgeSettings]
bEnableSOTSCoreLifecycleBridge=True
bEnableSOTSCoreBridgeVerboseLogs=True
```

## Behavior (when enabled)
- Registers a Core lifecycle listener via `FSOTS_CoreLifecycleListenerHandle`.
- On `OnSOTS_WorldStartPlay`:
  - Caches the active `USOTS_SuiteDebugSubsystem` from the world’s `UGameInstance`.
  - Binds travel delegates **only if** Core reports `IsMapTravelBridgeBound()`.
- On `PreLoadMap` (travel only):
  - Calls `USOTS_SuiteDebugSubsystem::HideKEMAnchorOverlay()` as a safe reset (removes overlay; does not create anything).

## Shipping/Test safety
- In **Shipping/Test**, the bridge does not register at module startup.

## Evidence
- Core travel bridge gate exists and is queryable:
  - `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()`.
- SOTS_Debug already has a safe teardown seam:
  - `USOTS_SuiteDebugSubsystem::HideKEMAnchorOverlay()`.

## Files
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugModule.cpp`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreLifecycleHook.*`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Public/SOTS_DebugCoreBridgeSettings.h`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreBridgeSettings.cpp`
- `Plugins/SOTS_Debug/SOTS_Debug.uplugin`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/SOTS_Debug.Build.cs`
