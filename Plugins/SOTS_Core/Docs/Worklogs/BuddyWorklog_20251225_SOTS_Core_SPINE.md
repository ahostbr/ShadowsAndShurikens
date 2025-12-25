# Buddy Worklog 2025-12-25 - SOTS_Core SPINE

## RepoIntel Evidence (RAG-first)
- SOTS_Core plugin existence: UNKNOWN in repo index cache (no SOTS_Core hits in DevTools/python/sots_repo_index/_cache/repo_index_cache.json); plugin catalog does not list SOTS_Core in Plugins/README.md:25, Plugins/README.md:41, Plugins/README.md:61.
- Existing ASOTS_* classes are limited to KEM/UDS helpers (no GameMode/GameState/PlayerState/PlayerController/HUD/PlayerCharacter bases):
  - DevTools/python/sots_repo_index/_cache/repo_index_cache.json:49387, DevTools/python/sots_repo_index/_cache/repo_index_cache.json:49712,
    DevTools/python/sots_repo_index/_cache/repo_index_cache.json:50739, DevTools/python/sots_repo_index/_cache/repo_index_cache.json:50831,
    DevTools/python/sots_repo_index/_cache/repo_index_cache.json:56468.
- Lifecycle/delegate pattern reference points:
  - USOTS_ProfileSubsystem uses UGameInstanceSubsystem + BlueprintAssignable delegate pattern in Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h:33 and Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h:15.
  - USOTS_UIRouterSubsystem defines dynamic multicast delegates in Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h:68 and Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h:188.

## Files Created (SOTS_Core)
- Plugin: Plugins/SOTS_Core/SOTS_Core.uplugin
- Module: Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs; Plugins/SOTS_Core/Source/SOTS_Core/Public/SOTS_Core.h; Plugins/SOTS_Core/Source/SOTS_Core/Private/SOTS_Core.cpp
- Lifecycle subsystem: Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h; Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Base classes:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/GameFramework/SOTS_GameModeBase.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_GameModeBase.cpp
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/GameFramework/SOTS_GameStateBase.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_GameStateBase.cpp
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/GameFramework/SOTS_PlayerStateBase.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_PlayerStateBase.cpp
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/GameFramework/SOTS_PlayerControllerBase.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_PlayerControllerBase.cpp
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/GameFramework/SOTS_HUDBase.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/GameFramework/SOTS_HUDBase.cpp
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Characters/SOTS_PlayerCharacterBase.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Characters/SOTS_PlayerCharacterBase.cpp
- Docs: Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md

## Delegate Surface
- Native: FSOTS_OnWorldStartPlay_Native, FSOTS_OnPostLogin_Native, FSOTS_OnLogout_Native, FSOTS_OnPlayerControllerBeginPlay_Native,
  FSOTS_OnPawnPossessed_Native, FSOTS_OnHUDReady_Native.
- Blueprint: FSOTS_OnWorldStartPlay_BP, FSOTS_OnPostLogin_BP, FSOTS_OnLogout_BP, FSOTS_OnPlayerControllerBeginPlay_BP,
  FSOTS_OnPawnPossessed_BP, FSOTS_OnHUDReady_BP.

## Null-safety / Double-fire Guards
- Notify_* functions return early on null pointers before logging/broadcast.
- Base classes use a local GetLifecycleSubsystem helper that null-checks World/GameInstance before use.
- PlayerController uses OnPossess only (no SetPawn hook), so no duplicate possession broadcasts.
