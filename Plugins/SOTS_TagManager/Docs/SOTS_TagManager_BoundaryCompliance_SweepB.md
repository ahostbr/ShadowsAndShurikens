# SOTS TagManager Boundary Compliance (Sweep B)

## What counts as boundary tags
Boundary tags are shared runtime actor-state tags that:
- Are applied to shared actors (Player, Dragon, Guard, Pickup) and
- Are expected to be observed or gated by other plugins.

Examples in the suite include:
- GSM stealth state tags (`SOTS.Stealth.State.*`, `SOTS.Stealth.Light.*`)
- AI awareness/focus/reason tags (`SAS.AI.Alert.*`, `SAS.AI.Focus.*`, `SAS.AI.Reason.*`)
- BodyDrag state tags (`SAS.BodyDrag.State.*`, `SAS.BodyDrag.Target.*`)

Internal or local tags that stay inside a single plugin (or exist only as asset metadata) are not boundary tags and can remain direct.

## Canonical read/write APIs
Use TagManager or TagLibrary for all boundary reads/writes:
- Union reads: `USOTS_GameplayTagManagerSubsystem::GetActorTags`, `ActorHasTag`, `ActorHasAnyTags`, `ActorHasAllTags`.
- Loose writes: `AddTagToActor`, `RemoveTagFromActor`, `RemoveTagsFromActor`.
- Scoped writes (temporary state): `AddScopedTagToActor` / `AddScopedLooseTag` + `RemoveScopedTagByHandle`.
- Blueprint access: `USOTS_TagLibrary` equivalents (all functions take a world context).

These APIs return the union of owned tags, loose tags, and scoped handle tags so cross-plugin consumers see a single coherent view.

## Common bypass patterns and replacements
- Direct `IGameplayTagAssetInterface::GetOwnedGameplayTags` reads -> `GetActorTags` from TagManager.
- Direct `AddLooseGameplayTag` / `RemoveLooseGameplayTag` on actors/components -> `AddTagToActor` / `RemoveTagFromActor`.
- Direct `FGameplayTagContainer::AddTag/RemoveTag` on shared actor caches -> TagManager Add/Remove or scoped handles.
- Direct `HasMatchingGameplayTag` or similar checks on shared actors -> TagManager `ActorHasTag` (or `ActorHasAnyTags` / `ActorHasAllTags`).

If the tag is boundary-visible, route through TagManager to preserve union semantics, tag-change events, and EndPlay cleanup guarantees.
