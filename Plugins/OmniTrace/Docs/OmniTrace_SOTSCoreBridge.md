# OmniTrace ↔ SOTS_Core Bridge (BRIDGE19)

## Goal
Provide an **opt-in**, **debug-only in spirit** integration point so OmniTrace can participate in deterministic SOTS_Core lifecycle callbacks (and optionally map travel callbacks), without changing gameplay behavior.

## Non-goals
- No Blueprint edits.
- No UE build/run verification by Buddy.
- No SaveContract participant (OmniTrace appears debug/state-light; no proven persistence seam).

## Settings
Settings live in `Project Settings -> OmniTrace - Core Bridge`.

Class: `UOmniTraceCoreBridgeSettings`

- `bEnableSOTSCoreLifecycleBridge` (default **false**)
- `bEnableSOTSCoreBridgeVerboseLogs` (default **false**)

Config example:
```ini
[/Script/OmniTrace.OmniTraceCoreBridgeSettings]
bEnableSOTSCoreLifecycleBridge=True
bEnableSOTSCoreBridgeVerboseLogs=True
```

## What the bridge does (when enabled)
- Registers an `ISOTS_CoreLifecycleListener` implementation (`FOmniTrace_CoreLifecycleHook`) via `FSOTS_CoreLifecycleListenerHandle`.
- On `OnSOTS_WorldStartPlay`:
  - Caches the active `UWorld` and `UOmniTraceDebugSubsystem`.
  - Attempts to bind travel delegates **only if** `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()` is true.
- On travel (`PreLoadMap`):
  - Clears OmniTrace’s last KEM debug record via `UOmniTraceDebugSubsystem::ClearLastKEMTrace()`.
  - Drops cached pointers.
- On travel (`PostLoadMap`):
  - Re-caches pointers for the new world.

## Shipping/Test safety
- In **Shipping/Test**, the bridge is **not registered** at module startup.

## Evidence (RAG-first)
- Latest RAG query output for this pass:
  - `Reports/RAG/rag_query_20251226_150920.txt`
  - `Reports/RAG/rag_query_20251226_150920.json`

## Files
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTrace.cpp`
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreLifecycleHook.h`
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreLifecycleHook.cpp`
- `Plugins/OmniTrace/Source/OmniTrace/Public/OmniTraceCoreBridgeSettings.h`
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreBridgeSettings.cpp`
- `Plugins/OmniTrace/Source/OmniTrace/Public/OmniTraceDebugSubsystem.h`
- `Plugins/OmniTrace/OmniTrace.uplugin`
- `Plugins/OmniTrace/Source/OmniTrace/OmniTrace.Build.cs`
