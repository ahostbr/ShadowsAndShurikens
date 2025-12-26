# BRIDGE20 — SOTS_Debug ↔ SOTS_Core Lifecycle Bridge

## Title
BRIDGE20: SOTS_Debug optional SOTS_Core lifecycle bridge (debug readiness + travel reset)

## Scope
- Plugin(s)/Tool(s): `SOTS_Debug`
- Pass: `SOTS_Core BRIDGE20 (SOTS_Debug)`
- Constraints: add-only; default OFF; no BP edits; no UE build/run

## Evidence (RAG-first + direct reads)
- Core travel gate and delegates exist:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h` (lines 105–140)
  - `Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp` (lines 118–150)
- SOTS_Debug has a safe reset seam for travel:
  - `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp` (lines 315–380)
- Core listener integration guidance:
  - `Plugins/SOTS_Core/Docs/SOTS_Core_LifecycleListener_Integration.md` (illustrative pseudocode)

## Changes
- Dependency wiring:
  - `Plugins/SOTS_Debug/SOTS_Debug.uplugin` adds `SOTS_Core` plugin dependency.
  - `Plugins/SOTS_Debug/Source/SOTS_Debug/SOTS_Debug.Build.cs` adds private deps: `DeveloperSettings`, `SOTS_Core`.
- Added opt-in settings (default OFF):
  - `USOTS_DebugCoreBridgeSettings` with:
    - `bEnableSOTSCoreLifecycleBridge=false`
    - `bEnableSOTSCoreBridgeVerboseLogs=false`
- Added lifecycle listener + minimal travel reset:
  - `FSOTS_DebugCoreLifecycleHook : ISOTS_CoreLifecycleListener`
  - On `WorldStartPlay`, caches `USOTS_SuiteDebugSubsystem` and binds travel delegates only when `IsMapTravelBridgeBound()` is true.
  - On `PreLoadMap`, calls `HideKEMAnchorOverlay()` (safe teardown; no new widget creation).
- Module wiring:
  - `FSOTS_DebugModule` conditionally registers a `FSOTS_CoreLifecycleListenerHandle` only when enabled, and is compiled out in Shipping/Test.

## Verification
- Static check only (no Unreal build/run):
  - VS Code diagnostics on touched files (no errors).

## Notes / Risks / Unknowns
- The bridge intentionally does not auto-show any debug widget; it only caches the subsystem and performs a safe teardown on travel.
- If Core travel bridge is not bound, the bridge remains WorldStartPlay-only.

## Files changed/created
- `Plugins/SOTS_Debug/SOTS_Debug.uplugin`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/SOTS_Debug.Build.cs`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugModule.cpp`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Public/SOTS_DebugCoreBridgeSettings.h`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreBridgeSettings.cpp`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreLifecycleHook.h`
- `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreLifecycleHook.cpp`
- `Plugins/SOTS_Debug/Docs/SOTS_Debug_SOTSCoreBridge.md`

## Followups / Next steps (Ryan)
- Enable `SOTS Debug - Core Bridge` and confirm listener registration (enable verbose logs if desired).
- If `IsMapTravelBridgeBound()` is true, travel between maps and confirm KEM overlay is removed on PreLoadMap.
- Confirm no behavior impact in Shipping/Test.
