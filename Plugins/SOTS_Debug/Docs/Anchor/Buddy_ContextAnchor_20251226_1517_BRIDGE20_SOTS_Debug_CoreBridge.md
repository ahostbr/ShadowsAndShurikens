Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1517 | Plugin: SOTS_Debug | Pass/Topic: BRIDGE20_CoreBridge | Owner: Buddy
Scope: Optional SOTS_Debug ↔ SOTS_Core lifecycle bridge (default OFF) with safe travel teardown

DONE
- Added `USOTS_DebugCoreBridgeSettings` (`bEnableSOTSCoreLifecycleBridge`, `bEnableSOTSCoreBridgeVerboseLogs`) default OFF.
- Added `FSOTS_DebugCoreLifecycleHook : ISOTS_CoreLifecycleListener` caching `USOTS_SuiteDebugSubsystem` on `WorldStartPlay`.
- Bound Core travel delegates only when `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()` is true.
- On `PreLoadMap`, calls `USOTS_SuiteDebugSubsystem::HideKEMAnchorOverlay()` (safe teardown only).
- Wired `FSOTS_DebugModule` to register `FSOTS_CoreLifecycleListenerHandle` only when enabled; compiled out in Shipping/Test.
- Added SOTS_Debug dependency declarations for `SOTS_Core` (uplugin + Build.cs).

VERIFIED
- No Unreal build/run verification performed by Buddy.

UNVERIFIED / ASSUMPTIONS
- Assumed hiding the KEM anchor overlay on travel is a desirable reset for debug UX.

FILES TOUCHED
- Plugins/SOTS_Debug/SOTS_Debug.uplugin
- Plugins/SOTS_Debug/Source/SOTS_Debug/SOTS_Debug.Build.cs
- Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugModule.cpp
- Plugins/SOTS_Debug/Source/SOTS_Debug/Public/SOTS_DebugCoreBridgeSettings.h
- Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreBridgeSettings.cpp
- Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreLifecycleHook.h
- Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_DebugCoreLifecycleHook.cpp
- Plugins/SOTS_Debug/Docs/SOTS_Debug_SOTSCoreBridge.md

NEXT (Ryan)
- Enable `SOTS Debug - Core Bridge` and optionally enable verbose logs.
- If `IsMapTravelBridgeBound()` is true, change maps and confirm `HideKEMAnchorOverlay()` occurs on PreLoadMap.
- Confirm Shipping/Test builds do not register bridge code.

ROLLBACK
- Revert the SOTS_Debug files listed above (and remove the new Docs files).