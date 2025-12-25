# Buddy Worklog 2025-12-25 - SOTS_Core SPINE3

## RepoIntel Evidence (RAG-first)
- SPINE1/SPINE2 symbols present in workspace:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h:54
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Settings/SOTS_CoreSettings.h:8
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Blueprint/SOTS_CoreBlueprintLibrary.h:13
- No existing lifecycle-listener interface found in repo search (rg: "ISOTS_CoreLifecycleListener|CoreLifecycleListener") -> NONE.
- ModularFeatures usage patterns: none found in repo search (rg: "IModularFeatures|RegisterModularFeature") -> UNKNOWN.
- DeveloperSettings patterns referenced:
  - Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h:16 (UCLASS Config=Game DefaultConfig)
  - Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputLayerRegistrySettings.h:22 (UCLASS Config=Game DefaultConfig)

## New Settings Flags (defaults)
- bEnableLifecycleListenerDispatch = false
- bEnableLifecycleDispatchLogs = false

## Dispatch Ordering
- Notify_* updates snapshot + duplicate suppression -> broadcasts native + BP delegates -> dispatches to listeners.
- Dispatch is gated by bEnableLifecycleListenerDispatch and ModularFeatures availability.

## Files Created/Modified
- Created:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListener.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListenerHandle.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Lifecycle/SOTS_CoreLifecycleListenerHandle.cpp
  - Plugins/SOTS_Core/Docs/SOTS_Core_LifecycleListener_Integration.md
- Modified:
  - Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Settings/SOTS_CoreSettings.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
  - Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md
