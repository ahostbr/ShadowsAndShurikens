# TagManager Scoped Loose Tags (2025-12-17 13:35)

Goal
- Add Blueprint-friendly scoped loose tag handles and lifecycle-safe events without breaking legacy loose tag APIs.

Changes
- Added `FSOTS_LooseTagHandle` Blueprint type for per-handle scoped loose tag references.
- USOTS_GameplayTagManagerSubsystem now tracks scoped counts, handle-to-record mapping, union-aware tag queries, and broadcasts OnLooseTagAdded/Removed on state transitions; added EndPlay cleanup and actor binding.
- Added scoped add/remove (by tag and by name) plus handle-based removal; unscoped adds/removes now gate events on visibility changes while preserving existing signatures.
- Exposed scoped APIs through USOTS_TagLibrary with world-context forwarding.

Files Changed
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_LooseTagHandle.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_GameplayTagManagerSubsystem.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_GameplayTagManagerSubsystem.cpp
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_TagLibrary.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_TagLibrary.cpp

Notes / Risks / Unknowns
- Cleanup is bound to OnEndPlay (covers Destroy) but not separately to OnDestroyed; assumes OnEndPlay always fires before destruction finalizes.
- OnLooseTagRemoved broadcasts best-effort union removals during EndPlay (includes interface-owned tags); runtime behavior UNVERIFIED.
- No build/editor run in this pass.

Verification
- UNVERIFIED (no compile/editor/runtime executed).

Cleanup
- Deleted Plugins/SOTS_TagManager/Binaries and Plugins/SOTS_TagManager/Intermediate after changes.

Follow-ups / TODO
- Route direct tag reads/writes found via DevTools/ad_hoc_regex_search.py through TagManager/TagLibrary where appropriate:
  - SOTS_Steam: Source/SOTS_Steam/Private/SOTS_SteamAchievementsSubsystem.cpp (MissionTags.HasAll), Source/SOTS_Steam/Private/SOTS_SteamLeaderboardsSubsystem.cpp (MissionTags.HasAll).
  - SOTS_MissionDirector: Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp (FailOnEventTags.HasAny on ContextTags).
  - SOTS_KillExecutionManager: Source/SOTS_KillExecutionManager/Private/SOTS_KEM_ManagerSubsystem.cpp (Context.ContextTags.HasAll/HasAny), Source/SOTS_KillExecutionManager/Private/SOTS_KEM_TagSelectionCriterion.cpp (HasTagExact on Primary/Querier/Context tags).
  - SOTS_GAS_Plugin: Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityGatingLibrary.cpp (HasAll/HasAny on skill tags), Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityRequirementLibrary.cpp (GetOwnedGameplayTags + HasAll on requirements), Source/SOTS_GAS_Plugin/Private/SOTS_GAS_DebugLibrary.cpp (GetOwnedGameplayTags on player tags).
  - NinjaInput: Source/NinjaInput/Public/NinjaInputHandlerHelpers.h (GetOwnedGameplayTags), Source/NinjaInput/Public/Types/FInputActionHandlerCache.h (HasAny), Source/NinjaInput/Private/InputHandlers/InputHandler_AbilityActivation.cpp (HasTagExact on ability tags).
- Consider adding explicit OnDestroyed binding if we need redundancy beyond OnEndPlay.
