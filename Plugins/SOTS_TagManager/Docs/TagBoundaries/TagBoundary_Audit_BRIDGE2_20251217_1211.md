# TagBoundary Audit BRIDGE2 (20251217_1211)

## Scope
- Root: Plugins
- Patterns: AddLooseGameplayTag, RemoveLooseGameplayTag, AddTag(, RemoveTag(, GetOwnedGameplayTags(, HasTagExact, HasAny(, HasAll(, FGameplayTagContainer::AddTag, FGameplayTagContainer::RemoveTag
- Patterns: RequestGameplayTag, UE_DEFINE_GAMEPLAY_TAG
- Seed namespaces: Input.Layer.*, Input.Intent.*, SAS.Stealth.*, SAS.UI.*, ability gating, inventory gating

## MUST CHANGE (fixed)
1) Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityRequirementLibrary.cpp
   - Issue: Direct player tag reads via IGameplayTagAssetInterface::GetOwnedGameplayTags for RequiredPlayerTags gating.
   - Why boundary: Ability requirements consume shared player state that other plugins publish.
   - Fix: Use TagManager via USOTS_TagLibrary::ActorHasAllTags(WorldContextObject, PlayerActor, Requirements.RequiredPlayerTags).

## OK AS-IS
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityGatingLibrary.cpp
  - Uses TagManager subsystem for player tag queries (boundary compliant).
- Plugins/SOTS_KillExecutionManager/Source/SOTS_KillExecutionManager/Private/*.cpp
  - Context tag containers are internal to KEM flow (not shared actor state).
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp
  - Context tags are local mission logic, not actor tag containers.
- Plugins/SOTS_Steam/Source/SOTS_Steam/Private/*.cpp
  - Mission result tags are metadata for achievements/leaderboards.
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
  - UI intent tags are internal router intents, not shared actor state.

## UNCERTAIN (needs owner decision)
- Plugins/NinjaInpaf825c9494a8V5/Source/NinjaInput/*
  - Uses GameplayTag containers and GetOwnedGameplayTags on AbilitySystemComponent. If these tags are intended as shared actor state, consider routing through TagManager or an adapter.
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_DebugLibrary.cpp
  - Debug-only read of player tags via IGameplayTagAssetInterface. If you want strict boundary compliance in debug tools, switch to TagManager queries.
