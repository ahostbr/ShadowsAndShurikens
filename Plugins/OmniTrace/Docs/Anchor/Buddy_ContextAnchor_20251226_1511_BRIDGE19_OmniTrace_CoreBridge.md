Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1511 | Plugin: OmniTrace | Pass/Topic: BRIDGE19_CoreBridge | Owner: Buddy
Scope: Optional OmniTrace ↔ SOTS_Core lifecycle bridge (default OFF) with travel-reset gating

DONE
- Added `UOmniTraceCoreBridgeSettings` (`bEnableSOTSCoreLifecycleBridge`, `bEnableSOTSCoreBridgeVerboseLogs`) default OFF.
- Added `FOmniTrace_CoreLifecycleHook : ISOTS_CoreLifecycleListener` and optional travel binding gated by `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()`.
- Added `UOmniTraceDebugSubsystem::ClearLastKEMTrace()` and clear call on `PreLoadMap`.
- Wired module `FOmniTraceModule` to register `FSOTS_CoreLifecycleListenerHandle` only when enabled; compiled out in Shipping/Test.
- Added OmniTrace plugin dependency on `SOTS_Core` and Build.cs private deps.

VERIFIED
- No Unreal build/run verification performed by Buddy.

UNVERIFIED / ASSUMPTIONS
- Assumed clearing `LastKEMTrace` on travel is the desired/only safe reset seam for OmniTrace.
- Assumed Core travel bridge binding (`IsMapTravelBridgeBound`) reflects real project travel events.

FILES TOUCHED
- Plugins/OmniTrace/OmniTrace.uplugin
- Plugins/OmniTrace/Source/OmniTrace/OmniTrace.Build.cs
- Plugins/OmniTrace/Source/OmniTrace/Private/OmniTrace.cpp
- Plugins/OmniTrace/Source/OmniTrace/Public/OmniTraceDebugSubsystem.h
- Plugins/OmniTrace/Source/OmniTrace/Public/OmniTraceCoreBridgeSettings.h
- Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreBridgeSettings.cpp
- Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreLifecycleHook.h
- Plugins/OmniTrace/Source/OmniTrace/Private/OmniTraceCoreLifecycleHook.cpp
- Plugins/OmniTrace/Docs/OmniTrace_SOTSCoreBridge.md

NEXT (Ryan)
- Enable `OmniTrace - Core Bridge` settings and confirm listener registers (verbose logs on).
- If `IsMapTravelBridgeBound()` is true, change maps and verify `ClearLastKEMTrace()` occurs on PreLoadMap.
- Confirm Shipping/Test builds do not register bridge code.

ROLLBACK
- Revert the OmniTrace files listed above (and remove the new Docs files).