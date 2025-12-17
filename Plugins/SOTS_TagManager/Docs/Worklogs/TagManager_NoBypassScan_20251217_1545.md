# TagManager No-Bypass Scan (2025-12-17 15:45)

Tooling
- DevTools/python/ad_hoc_regex_search.py
- Pattern: GetOwnedGameplayTags(|IGameplayTagAssetInterface|AddLooseGameplayTag|RemoveLooseGameplayTag|HasTagExact|HasAny(|HasAll(
- Root: Plugins, Context: 1 line, Files scanned: 1493, Matches: 38, Files with matches: 16 (RUN_ID=e69f6277-c950-4148-bc53-2f8315fd0e6b)

Findings (needs routing through TagManager/TagLibrary in future passes)
- SOTS_Steam: Source/SOTS_Steam/Private/SOTS_SteamAchievementsSubsystem.cpp (MissionTags.HasAll), Source/SOTS_Steam/Private/SOTS_SteamLeaderboardsSubsystem.cpp (MissionTags.HasAll)
- SOTS_MissionDirector: Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp (FailOnEventTags.HasAny on ContextTags)
- SOTS_KillExecutionManager: Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp (Context.ContextTags HasAll/HasAny), Source/SOTS_KillExecutionManager/Private/SOTS_KEM_TagSelectionCriterion.cpp (HasTagExact on Primary/Querier/Context)
- SOTS_GAS_Plugin: Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityGatingLibrary.cpp (HasAll/HasAny), Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityRequirementLibrary.cpp (GetOwnedGameplayTags + HasAll), Source/SOTS_GAS_Plugin/Private/SOTS_GAS_DebugLibrary.cpp (GetOwnedGameplayTags)
- NinjaInput: Source/NinjaInput/Public/NinjaInputHandlerHelpers.h (GetOwnedGameplayTags), Source/NinjaInput/Public/Types/FInputActionHandlerCache.h (HasAny), Source/NinjaInput/Private/Components/NinjaInputBufferComponent.cpp (HasTagExact), Source/NinjaInput/Private/Components/NinjaInputManagerComponent.cpp (HasAny), Source/NinjaInput/Private/Input/Handlers/InputHandler_AbilityActivation.cpp (HasTagExact on ability tags)

Notes
- TagManager files matched (expected) and are already under control; not listed as bypass issues.
- No refactors performed in this pass; this document is for routing queue in future migration sweeps.
