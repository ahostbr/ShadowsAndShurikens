# Buddy Worklog — 20251226_122600 — BRIDGE12_GSM_CoreLifecycleBridge

## Goal
Implement BRIDGE12: an OFF-by-default, state-only lifecycle bridge from `SOTS_Core` to `SOTS_GlobalStealthManager` using the `ISOTS_CoreLifecycleListener` modular feature.

## What changed
- Added `SOTS_Core` module dependency and plugin dependency for `SOTS_GlobalStealthManager`.
- Added `USOTS_GlobalStealthManagerCoreBridgeSettings` developer settings (default OFF) to gate registration and verbose logging.
- Added `FGSM_CoreLifecycleHook : ISOTS_CoreLifecycleListener` and registration/unregistration in the GSM module.
- Added state-only handlers and native multicast delegates to `USOTS_GlobalStealthManagerSubsystem`:
  - `HandleCoreWorldReady(UWorld*)` + `OnCoreWorldReady`
  - `HandleCorePrimaryPlayerReady(APlayerController*, APawn*)` + `OnCorePrimaryPlayerReady`

## Files changed
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

## Notes / Risks / Unknowns
- UNVERIFIED: No build/editor run performed (per project law). This follows the established pattern used by `SOTS_AIPerception`.
- Registration only happens when enabled at startup; toggling at runtime requires restart (expected).

## Verification
- VERIFIED (static): No VS Code errors reported in edited files.
- UNVERIFIED (runtime): UE module compilation and in-editor behavior.

## Follow-ups / Next steps
- Ryan: enable the setting and confirm callbacks fire in a PIE session.
- If needed later: decide whether GSM should also forward Core travel delegates (not part of BRIDGE12).
