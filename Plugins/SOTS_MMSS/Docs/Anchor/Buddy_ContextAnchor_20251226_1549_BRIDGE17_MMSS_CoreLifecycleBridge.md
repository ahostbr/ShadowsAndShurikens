Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1549 | Plugin: SOTS_MMSS | Pass/Topic: BRIDGE17_MMSS_CoreLifecycleBridge | Owner: Buddy
Scope: Optional SOTS_MMSS ↔ SOTS_Core lifecycle + travel relay bridge (state-only, OFF by default)

DONE
- Added `USOTS_MMSSCoreBridgeSettings` (Project Settings; default OFF) to gate MMSS Core lifecycle bridge.
- Added `SOTS_MMSS::CoreBridge` native delegates: `OnCoreWorldReady_Native`, `OnCorePrimaryPlayerReady_Native`, `OnCorePreLoadMap_Native`, `OnCorePostLoadMap_Native`.
- Added `FMMSS_CoreLifecycleHook : ISOTS_CoreLifecycleListener` broadcasting state-only events and binding travel relays only when `USOTS_CoreLifecycleSubsystem::IsMapTravelBridgeBound()` is true.
- Updated MMSS module startup/shutdown to register/unregister the listener (register only if settings enabled).
- Updated deps: `SOTS_MMSS.Build.cs` adds `DeveloperSettings` + `SOTS_Core` (private); `SOTS_MMSS.uplugin` declares plugin dep `SOTS_Core`.

VERIFIED
- Static error scan: no errors found in the touched MMSS bridge files.

UNVERIFIED / ASSUMPTIONS
- Not build/run verified in Unreal Editor.
- Assumes Core travel delegates are active only when `IsMapTravelBridgeBound()` returns true (MMSS binds only in that case).

FILES TOUCHED
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/SOTS_MMSS.Build.cs
- Plugins/SOTS_MMSS/SOTS_MMSS.uplugin
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSModule.cpp
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSSCoreBridgeSettings.h
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreBridgeSettings.cpp
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSSCoreBridgeDelegates.h
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreBridgeDelegates.cpp
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreLifecycleHook.h
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreLifecycleHook.cpp
- Plugins/SOTS_MMSS/Docs/SOTS_MMSS_SOTSCoreBridge.md
- Plugins/SOTS_MMSS/Docs/Worklogs/BuddyWorklog_20251226_154900_BRIDGE17_MMSS_CoreLifecycleBridge.md
- Plugins/SOTS_MMSS/Docs/Anchor/Buddy_ContextAnchor_20251226_1549_BRIDGE17_MMSS_CoreLifecycleBridge.md

NEXT (Ryan)
- Enable `SOTS_MMSS - Core Bridge` setting and confirm delegate broadcasts on WorldStartPlay and PawnPossessed.
- Confirm duplicate suppression works (same PC/Pawn doesn’t rebroadcast).
- If Core travel bridge is enabled, confirm PreLoad/PostLoad relays fire and MMSS cached primary-player pointers clear on PreLoad.

ROLLBACK
- Revert the touched files listed above, or disable the MMSS Core Bridge setting (default is OFF).
