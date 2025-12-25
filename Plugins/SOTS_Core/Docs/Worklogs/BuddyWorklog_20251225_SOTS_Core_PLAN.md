# Buddy Worklog 2025-12-25 - SOTS_Core PLAN

## RepoIndex Evidence (preflight)
- Existing ASOTS_* classes are limited to KEM/UDS helpers (no GameMode/GameState/PlayerState/PlayerController/HUD/PlayerCharacter bases found in cache search): DevTools/python/sots_repo_index/_cache/repo_index_cache.json:49387, DevTools/python/sots_repo_index/_cache/repo_index_cache.json:49712, DevTools/python/sots_repo_index/_cache/repo_index_cache.json:50739, DevTools/python/sots_repo_index/_cache/repo_index_cache.json:50831, DevTools/python/sots_repo_index/_cache/repo_index_cache.json:56468.
- Plugin catalog lists existing SOTS plugins but does not include SOTS_Core: Plugins/README.md:25, Plugins/README.md:41, Plugins/README.md:61.
- Lifecycle/delegate patterns in use: USOTS_ProfileSubsystem is a UGameInstanceSubsystem and exposes a BlueprintAssignable OnProfileRestored delegate: Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h:33, Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h:15, Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h:58.
- UI subsystem uses dynamic multicast delegates for UI lifecycle events (reference for pattern only): Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h:68, Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h:188, Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h:189, Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h:190.

## Plan
1) Add SOTS_Core runtime plugin skeleton (.uplugin, module boilerplate, Build.cs).
2) Implement USOTS_CoreLifecycleSubsystem with native + BlueprintAssignable delegates and Notify_* helpers.
3) Add UE-native base classes (GameMode/GameState/PlayerState/PlayerController/HUD/PlayerCharacter) that only forward lifecycle notifications.
4) Add docs (overview + adoption guidance) and confirm no dependencies on other SOTS plugins.
