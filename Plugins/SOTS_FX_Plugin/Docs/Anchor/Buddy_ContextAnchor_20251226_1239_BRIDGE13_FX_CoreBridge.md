Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1239 | Plugin: SOTS_FX_Plugin | Pass/Topic: BRIDGE13_CoreLifecycleBridge | Owner: Buddy
Scope: Add OFF-by-default SOTS_Core lifecycle listener bridge into FX (world-ready + primary-player-ready), state-only.

DONE
- Added `SOTS_Core` dependency (Build.cs) and plugin dependency (uplugin) for SOTS_FX_Plugin.
- Added `USOTS_FXCoreBridgeSettings` with `bEnableSOTSCoreLifecycleBridge` and `bEnableSOTSCoreBridgeVerboseLogs` (default OFF).
- Added `FFX_CoreLifecycleHook : ISOTS_CoreLifecycleListener` and registration via `FSOTS_CoreLifecycleListenerHandle` in module Startup/Shutdown.
- Added state-only subsystem handlers + native delegates:
  - `USOTS_FXManagerSubsystem::HandleCoreWorldReady(UWorld*)` → `OnCoreWorldReady`
  - `USOTS_FXManagerSubsystem::HandleCorePrimaryPlayerReady(APlayerController*, APawn*)` → `OnCorePrimaryPlayerReady`

VERIFIED
- No runtime verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes SOTS_Core lifecycle events fire as expected and mirror the AIPerception/GSM bridge patterns.

FILES TOUCHED
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/SOTS_FX_Plugin.Build.cs
- Plugins/SOTS_FX_Plugin/SOTS_FX_Plugin.uplugin
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FX_PluginModule.cpp
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXCoreBridgeSettings.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXCoreBridgeSettings.cpp
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXCoreLifecycleHook.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXCoreLifecycleHook.cpp
- Plugins/SOTS_FX_Plugin/Docs/SOTS_FX_SOTSCoreBridge.md

NEXT (Ryan)
- Build and confirm SOTS_FX_Plugin compiles with the new SOTS_Core dependency.
- Enable Project Settings → "SOTS FX - Core Bridge" → `bEnableSOTSCoreLifecycleBridge`.
- PIE: confirm hook receives `OnSOTS_WorldStartPlay` and `OnSOTS_PawnPossessed` (use verbose logs toggle if needed).
- Confirm FX subsystem delegates fire and do not affect behavior when bridge is OFF.

ROLLBACK
- Revert the touched files listed above; then delete `Plugins/SOTS_FX_Plugin/Binaries` and `Plugins/SOTS_FX_Plugin/Intermediate` if present.
