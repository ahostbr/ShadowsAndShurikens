Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1226 | Plugin: SOTS_GlobalStealthManager | Pass/Topic: BRIDGE12_CoreLifecycleBridge | Owner: Buddy
Scope: Add OFF-by-default SOTS_Core lifecycle listener bridge into GSM (world-ready + primary-player-ready), state-only.

DONE
- Added `SOTS_Core` dependency (Build.cs) and plugin dependency (uplugin) for SOTS_GlobalStealthManager.
- Added `USOTS_GlobalStealthManagerCoreBridgeSettings` with `bEnableSOTSCoreLifecycleBridge` and `bEnableSOTSCoreBridgeVerboseLogs` (default OFF).
- Added `FGSM_CoreLifecycleHook : ISOTS_CoreLifecycleListener` and registration via `FSOTS_CoreLifecycleListenerHandle` in GSM module Startup/Shutdown.
- Added state-only subsystem handlers + native delegates:
  - `USOTS_GlobalStealthManagerSubsystem::HandleCoreWorldReady(UWorld*)` → `OnCoreWorldReady`
  - `USOTS_GlobalStealthManagerSubsystem::HandleCorePrimaryPlayerReady(APlayerController*, APawn*)` → `OnCorePrimaryPlayerReady`

VERIFIED
- No runtime verification performed.

UNVERIFIED / ASSUMPTIONS
- Assumes SOTS_Core lifecycle events fire as expected and mirror the AIPerception bridge pattern.

FILES TOUCHED
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/SOTS_GlobalStealthManager.Build.cs
- Plugins/SOTS_GlobalStealthManager/SOTS_GlobalStealthManager.uplugin
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerModule.cpp
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerCoreBridgeSettings.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerCoreBridgeSettings.cpp
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerCoreLifecycleHook.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerCoreLifecycleHook.cpp
- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_SOTSCoreBridge.md

NEXT (Ryan)
- Build the project/plugin and confirm SOTS_GlobalStealthManager compiles with the new SOTS_Core dependency.
- In Project Settings, enable "SOTS GlobalStealthManager - Core Bridge" → `bEnableSOTSCoreLifecycleBridge`.
- PIE: confirm `FGSM_CoreLifecycleHook` receives `OnSOTS_WorldStartPlay` and `OnSOTS_PawnPossessed` (use verbose logs toggle if needed).
- Confirm GSM subsystem delegates fire and do not affect gameplay when bridge is OFF.

ROLLBACK
- Revert the touched files listed above; then delete `Plugins/SOTS_GlobalStealthManager/Binaries` and `Plugins/SOTS_GlobalStealthManager/Intermediate` if present.
