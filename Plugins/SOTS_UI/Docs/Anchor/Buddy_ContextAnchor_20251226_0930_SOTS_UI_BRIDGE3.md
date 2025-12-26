Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_0930 | Plugin: SOTS_UI | Pass/Topic: BRIDGE3 HUDHost | Owner: Buddy
Scope: Add OFF-by-default SOTS_UI ↔ SOTS_Core bridge for HUDReady host registration

DONE
- Added SOTS_UI dependency on SOTS_Core (Build.cs).
- Added SOTS_UI settings: bEnableSOTSCoreHUDHostBridge (default false) and bEnableSOTSCoreBridgeVerboseLogs (default false).
- Registered a ModularFeatures listener implementing ISOTS_CoreLifecycleListener; on OnSOTS_HUDReady(AHUD*) it calls USOTS_UIRouterSubsystem::RegisterHUDHost.
- Added router seam: RegisterHUDHost(AHUD*), HasHUDHost(), GetHUDHostDebugString() storing a weak HUD pointer only.

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Assumes SOTS_Core listener dispatch is enabled separately (Core setting) for callbacks to fire.
- No runtime/editor verification performed.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/SOTS_UI.Build.cs
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIModule.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Docs/SOTS_UI_SOTSCoreBridge.md
- Plugins/SOTS_UI/Docs/Worklogs/BuddyWorklog_20251226_0930_SOTS_UI_BRIDGE3.md
- Plugins/SOTS_UI/Docs/Anchor/Buddy_ContextAnchor_20251226_0930_SOTS_UI_BRIDGE3.md

NEXT (Ryan)
- Build + run with Core dispatch ON and BRIDGE3 ON; success = router reports HUDHost valid and no UI stack changes occur.

ROLLBACK
- Revert the touched files above, or set SOTS_UI bEnableSOTSCoreHUDHostBridge=false.
