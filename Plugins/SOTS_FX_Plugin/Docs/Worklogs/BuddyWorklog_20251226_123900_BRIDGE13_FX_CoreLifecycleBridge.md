# Buddy Worklog — 20251226_123900 — BRIDGE13_FX_CoreLifecycleBridge

## Goal
Implement BRIDGE13: an OFF-by-default, state-only lifecycle bridge from `SOTS_Core` to `SOTS_FX_Plugin` using the `ISOTS_CoreLifecycleListener` modular feature.

## What changed
- Added `SOTS_Core` module dependency and plugin dependency for `SOTS_FX_Plugin`.
- Added `USOTS_FXCoreBridgeSettings` developer settings (default OFF) to gate registration and verbose logging.
- Added `FFX_CoreLifecycleHook : ISOTS_CoreLifecycleListener` and registration/unregistration in the FX module.
- Added state-only handlers and native multicast delegates to `USOTS_FXManagerSubsystem`:
  - `HandleCoreWorldReady(UWorld*)` + `OnCoreWorldReady`
  - `HandleCorePrimaryPlayerReady(APlayerController*, APawn*)` + `OnCorePrimaryPlayerReady`

## Files changed
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

## Notes / Risks / Unknowns
- UNVERIFIED: No build/editor run performed (per project law). This mirrors the established pattern used by AIPerception/GSM.
- Registration only happens when enabled at startup; runtime toggling requires restart.

## Verification
- VERIFIED (static): No VS Code errors reported in edited files.
- UNVERIFIED (runtime): UE module compilation and in-editor behavior.

## Follow-ups / Next steps
- Ryan: enable the setting and confirm callbacks fire in PIE. Use verbose logs toggle if needed.
