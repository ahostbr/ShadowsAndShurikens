Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1348 | Plugin: SOTS_AIPerception | Pass/Topic: BRIDGE11 CoreLifecycle | Owner: Buddy
Scope: Add OFF-by-default SOTS_AIPerception ↔ SOTS_Core lifecycle bridge (state-only)

DONE
- Added SOTS_AIPerception dependency on SOTS_Core (Build.cs + .uplugin plugin dep).
- Added USOTS_AIPerceptionCoreBridgeSettings with bEnableSOTSCoreLifecycleBridge and bEnableSOTS_CoreBridgeVerboseLogs (defaults false).
- Added FAIPerception_CoreLifecycleHook implementing ISOTS_CoreLifecycleListener; forwards WorldStartPlay and PawnPossessed to USOTS_AIPerceptionSubsystem handlers.
- Optional: binds to USOTS_CoreLifecycleSubsystem travel delegates (OnPreLoadMap_Native/OnPostLoadMap_Native) when Core bEnableMapTravelBridge=true.
- Added USOTS_AIPerceptionSubsystem state-only handlers + internal delegates: OnCoreWorldReady, OnCorePrimaryPlayerReady, OnCorePreLoadMap, OnCorePostLoadMap.

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Assumes SOTS_Core lifecycle dispatch is enabled separately for listener callbacks to fire.
- No runtime/editor verification performed.

FILES TOUCHED
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/SOTS_AIPerception.Build.cs
- Plugins/SOTS_AIPerception/SOTS_AIPerception.uplugin
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerception.cpp
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionSubsystem.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionSubsystem.cpp
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionCoreBridgeSettings.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionCoreBridgeSettings.cpp
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionCoreLifecycleHook.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionCoreLifecycleHook.cpp
- Plugins/SOTS_AIPerception/Docs/SOTS_AIPerception_SOTSCoreBridge.md
- Plugins/SOTS_AIPerception/Docs/Worklogs/BuddyWorklog_20251226_134800_BRIDGE11_SOTS_AIPerception_CoreLifecycleBridge.md
- Plugins/SOTS_AIPerception/Docs/Anchor/Buddy_ContextAnchor_20251226_1348_BRIDGE11_SOTS_AIPerception_CoreBridge.md

NEXT (Ryan)
- Build + run with Core dispatch ON + BRIDGE11 ON; success = delegates fire and nothing changes when BRIDGE11 is OFF.

ROLLBACK
- Revert the touched files above, or set bEnableSOTSCoreLifecycleBridge=false.
