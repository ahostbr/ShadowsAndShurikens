# Buddy Worklog 2025-12-25 - SOTS_Core SPINE5

## RepoIntel Evidence (RAG-first)
- FSOTS_CoreLifecycleSnapshot location:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h:40
- Existing identity/profile pattern in suite:
  - FSOTS_ProfileId in Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h:10

## Identity Notes
- No OnlineSubsystem dependencies added; identity is derived from PlayerController/PlayerState only.
- PlatformUserIdString omitted (engine type availability not proven without extra modules).

## Settings Added (defaults)
- bEnablePrimaryIdentityCache = true

## Files Modified
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Settings/SOTS_CoreSettings.h
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Blueprint/SOTS_CoreBlueprintLibrary.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Blueprint/SOTS_CoreBlueprintLibrary.cpp
- Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md
