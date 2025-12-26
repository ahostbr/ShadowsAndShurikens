# Buddy Worklog 2025-12-25 - SOTS_Core SPINE4

## RepoIntel Evidence (delegate availability)
- FCoreUObjectDelegates map load delegates:
  - C:\Program Files\Epic Games\UE_5.4\Engine\Source\Runtime\CoreUObject\Public\UObject\UObjectGlobals.h:3138 (FPreLoadMapDelegate)
  - C:\Program Files\Epic Games\UE_5.4\Engine\Source\Runtime\CoreUObject\Public\UObject\UObjectGlobals.h:3139 (PreLoadMap)
  - C:\Program Files\Epic Games\UE_5.4\Engine\Source\Runtime\CoreUObject\Public\UObject\UObjectGlobals.h:3147 (PostLoadMapWithWorld)
- FWorldDelegates world lifecycle delegates:
  - C:\Program Files\Epic Games\UE_5.4\Engine\Source\Runtime\Engine\Classes\Engine\World.h:4087 (OnPostWorldInitialization)
  - C:\Program Files\Epic Games\UE_5.4\Engine\Source\Runtime\Engine\Classes\Engine\World.h:4127 (OnWorldCleanup)
  - C:\Program Files\Epic Games\UE_5.4\Engine\Source\Runtime\Engine\Classes\Engine\World.h:2499-2500 (OnWorldBeginPlay)
- Seamless travel delegates (present but NOT wired in SPINE4):
  - C:\Program Files\Epic Games\UE_5.4\Engine\Source\Runtime\Engine\Classes\Engine\World.h:4174-4178 (OnSeamlessTravelStart / OnSeamlessTravelTransition)

## Settings Added (defaults)
- bEnableMapTravelBridge = false
- bEnableMapTravelLogs = false

## Dispatch Notes
- Map/travel events are gated by bEnableMapTravelBridge.
- Duplicate suppression uses cached last map/world pointers when bSuppressDuplicateNotifications is true.
- Map/travel logs are Verbose-only and gated by bEnableMapTravelLogs.

## Files Modified
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Settings/SOTS_CoreSettings.h
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md
