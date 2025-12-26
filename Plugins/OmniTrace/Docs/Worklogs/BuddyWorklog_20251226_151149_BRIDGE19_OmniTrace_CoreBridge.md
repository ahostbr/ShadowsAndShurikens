# BRIDGE19 — OmniTrace ↔ SOTS_Core Lifecycle Bridge

## Title
BRIDGE19: OmniTrace optional SOTS_Core lifecycle bridge

## Scope
- Plugin(s)/Tool(s): `OmniTrace`
- Pass: `SOTS_Core BRIDGE19 (OmniTrace)`
- Constraints: add-only; default OFF; no BP edits; no UE build/run; no extra UX; Shipping/Test safe

## Evidence
- RAG index refreshed (hash embeddings fallback).
- RAG query artifacts:
  - `Reports/RAG/rag_query_20251226_150920.txt`
  - `Reports/RAG/rag_query_20251226_150920.json`

## Changes
- Added SOTS_Core dependency wiring:
  - `Plugins/OmniTrace/OmniTrace.uplugin` now depends on `SOTS_Core`.
  - `Plugins/OmniTrace/Source/OmniTrace/OmniTrace.Build.cs` adds `SOTS_Core` + `DeveloperSettings` private deps.
- Added opt-in settings (default OFF):
  - `UOmniTraceCoreBridgeSettings` (`bEnableSOTSCoreLifecycleBridge`, `bEnableSOTSCoreBridgeVerboseLogs`).
- Added a minimal debug-reset seam:
  - `UOmniTraceDebugSubsystem::ClearLastKEMTrace()` to clear cached debug record.
- Added lifecycle listener + optional travel binding:
  - `FOmniTrace_CoreLifecycleHook : ISOTS_CoreLifecycleListener` caches subsystem on `WorldStartPlay`.
  - Binds travel delegates only if `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()`.
  - Clears last KEM trace on `PreLoadMap`.
- Module wiring:
  - `FOmniTraceModule::StartupModule()` registers the listener handle only when enabled, and is compiled out for Shipping/Test.

## Verification
- Static check only (no Unreal build/run):
  - VS Code diagnostics: no errors reported in touched OmniTrace bridge files.

## Notes / Risks / Unknowns
- OmniTrace appears to be primarily a debug utility; no proven persistence seam was found/used, so no SaveContract participant was added.
- Travel reset is gated by Core’s travel bridge binding; if `IsMapTravelBridgeBound()` is false, only `WorldStartPlay` caching occurs.

## Files changed
- `Plugins/OmniTrace/OmniTrace.uplugin`
- `Plugins/OmniTrace/Source/OmniTrace/OmniTrace.Build.cs`
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTrace.cpp`
- `Plugins/OmniTrace/Source/OmniTrace/Public/OmniTraceDebugSubsystem.h`
- `Plugins/OmniTrace/Source/OmniTrace/Public/OmniTraceCoreBridgeSettings.h`
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreBridgeSettings.cpp`
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreLifecycleHook.h`
- `Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreLifecycleHook.cpp`
- `Plugins/OmniTrace/Docs/OmniTrace_SOTSCoreBridge.md`

## Followups / Next steps (Ryan)
- In editor, enable `OmniTrace - Core Bridge` and confirm the listener registers.
- If Core travel bridge is bound, travel between maps and confirm last KEM trace clears on travel (optional verbose logs can help).
- Confirm no Shipping/Test behavior impact.
