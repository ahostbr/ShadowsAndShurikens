# Buddy Worklog 2025-12-25 - SOTS_Core SPINE2

## RepoIntel Evidence (RAG-first)
- SPINE1 class headers present in workspace (repo index cache predates SOTS_Core, so index evidence is UNKNOWN):
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/GameFramework/SOTS_GameModeBase.h:8
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/GameFramework/SOTS_PlayerControllerBase.h:8
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h:54
- Existing ASOTS_* classes in repo index cache are limited to KEM/UDS helpers (no naming conflicts with the new bases):
  - DevTools/python/sots_repo_index/_cache/repo_index_cache.json:49387
  - DevTools/python/sots_repo_index/_cache/repo_index_cache.json:49712
  - DevTools/python/sots_repo_index/_cache/repo_index_cache.json:50739
  - DevTools/python/sots_repo_index/_cache/repo_index_cache.json:50831
  - DevTools/python/sots_repo_index/_cache/repo_index_cache.json:56468
- DeveloperSettings patterns used in-suite:
  - Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h:16 (UCLASS Config=Game DefaultConfig)
  - Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h:24 (GetCategoryName override)
  - Plugins/SOTS_Input/Source/SOTS_Input/Public/SOTS_InputLayerRegistrySettings.h:22 (UCLASS Config=Game DefaultConfig)

## Settings (defaults)
- USOTS_CoreSettings:
  - bEnableWorldDelegateBridge = false
  - bEnableVerboseCoreLogs = false
  - bSuppressDuplicateNotifications = true

## Delegate Bridge Bindings
- Bound only when bEnableWorldDelegateBridge is true:
  - FWorldDelegates::OnPostWorldInitialization
  - FWorldDelegates::OnWorldBeginPlay
- UNKNOWN: FGameModeEvents::GameModePostLoginEvent / GameModeLogoutEvent availability (not bound).

## Duplicate Suppression Rules
- WorldStartPlay: only first broadcast per world instance.
- PlayerControllerBeginPlay: only once per PC instance (tracked set).
- PawnPossessed: only when PC/Pawn pair changes.
- HUDReady: only when HUD pointer changes.

## Files Created/Modified
- Created:
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Settings/SOTS_CoreSettings.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Settings/SOTS_CoreSettings.cpp
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Blueprint/SOTS_CoreBlueprintLibrary.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Blueprint/SOTS_CoreBlueprintLibrary.cpp
- Modified:
  - Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs
  - Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
  - Plugins/SOTS_Core/Source/SOTS_Core/Private/Characters/SOTS_PlayerCharacterBase.cpp
  - Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md
