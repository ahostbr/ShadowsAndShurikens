# TagBoundary Audit BRIDGE2 (20251217_1528)

## Scope
- Root: Plugins/** (code) — search via DevTools/ad_hoc_regex_search.py
- Patterns: AddLooseGameplayTag, RemoveLooseGameplayTag, AddTag(, RemoveTag(, GetOwnedGameplayTags(, HasTagExact, HasAny(, HasAll(, FGameplayTagContainer::AddTag, FGameplayTagContainer::RemoveTag
- Seed namespaces: Input.Layer.*, Input.Intent.*, SAS.Stealth.*, SAS.UI.*, ability/inventory gating tags

## MUST CHANGE
- None found.

## OK AS-IS
- Player tag gating in [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityRequirementLibrary.cpp#L131](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_AbilityRequirementLibrary.cpp#L131) already routes through `USOTS_TagLibrary::ActorHasAllTags` (boundary-compliant read for shared player tags).
- UI intent/tag checks in [Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp](Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp) are UI-internal lifecycle signals, not shared actor state.

## UNCERTAIN (owner decision)
- [Plugins/NinjaInpaf825c9494a8V5/Source/NinjaInput/Public/NinjaInputHandlerHelpers.h#L25](Plugins/NinjaInpaf825c9494a8V5/Source/NinjaInput/Public/NinjaInputHandlerHelpers.h#L25): Direct `GetOwnedGameplayTags` on the AbilitySystemComponent to satisfy tag queries. If `Input.*` tags are treated as shared runtime actor state across plugins, consider routing through TagManager (e.g., adapter that queries `USOTS_TagLibrary`) or documenting this as a local-only contract. If ability tags remain internal to NinjaInput, leave as-is.
- [Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_DebugLibrary.cpp#L109](Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/SOTS_GAS_DebugLibrary.cpp#L109): Debug-only read of player tags via `IGameplayTagAssetInterface`. Optional: swap to `USOTS_TagLibrary` for strict boundary hygiene if desired; currently logging only (shipping/test compiled out).

## Suggested fixes (only if escalated from UNCERTAIN)
- NinjaInput: add a small bridge helper (e.g., `UNinjaInputTagBridge::GetOwnedTagsThroughManager`) that defers to TagManager when available; keep existing path as fallback for local/internal tags.
- GAS Debug: replace direct `GetOwnedGameplayTags` with `USOTS_TagLibrary::ActorHasAllTags`/`ActorHasAnyTags` lookups when logging shared actor state.

## Notes
- No code changes made in this audit pass (report-only).
