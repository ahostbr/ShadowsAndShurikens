# SOTS TagManager Runtime Contract

## Boundary tag authority
- `USOTS_GameplayTagManagerSubsystem` is the canonical writer/reader for any shared actor-state tags that span plugins (Player, Dragon, Guard, Pickup, etc.). TagManager owns the `ActorLooseTags` cache and the union helpers documented in [`Docs/TagBoundary.md`](Plugins/SOTS_TagManager/Docs/TagBoundary.md) so other systems must call into this subsystem or the blueprint-friendly `USOTS_TagLibrary` before mutating those tags.
- Attack surfaces that observe or gate on Player/Dragon/Guard/Pickup tags should rely on TagManager’s tag handles/events rather than duplicating their own storage. The subsystem is accessible from any world context, so even gameplay systems without direct dependencies can resolve tags safely.

## Scoped handle semantics
- `AddScopedTagToActor`/`AddScopedLooseTag` return an `FSOTS_LooseTagHandle` whose GUID is stored in `HandleToRecord`. Each actor maintains counts (`ActorScopedTagCounts`) per tag, so multiple components can contribute the same tag without clobbering one another.
- `RemoveScopedTagByHandle` decrements the count, prunes zero-count entries, clears the lookup map, and fires `OnLooseTagRemoved` if the union-visible state flips from present to absent. `IsScopedHandleValid` lets callers guard against double-removal after handles become stale.
- Handles and counts are the only way to remove scoped contributions—never call `ActorLooseTags` directly—so scoped tags are scoped by design.

## Cleanup guarantees
- `EnsureEndPlayBinding` hooks `AActor::OnEndPlay` the first time the subsystem touches an actor. `HandleActorEndPlay` then clears `ActorLooseTags`, `ActorScopedTagCounts`, `HandleToRecord`, and unregisters the binding. This leaves no outstanding handles or leaked table entries once an actor dies (even if a handle was never explicitly removed).
- Before and after the cleanup sweep it computes the union (`BuildActorTagUnion`) so any tags that disappear due to cleanup broadcast `OnLooseTagRemoved`, keeping observers in sync.

## Event guarantees
- `OnLooseTagAdded`/`OnLooseTagRemoved` only fire when the union-visible state changes. Each mutator checks `UnionHasTag` before and after (`AddTagToActor`, `AddScopedTagToActor`, `RemoveTagFromActor`, `RemoveTagsFromActor`, `RemoveScopedTagByHandle`) so the same tag firing twice is impossible unless the visible count truly transitions (e.g., 0→1 or 1→0).
- Cleanup also walks the pre-clean union to broadcast removals for every tag that vanished, which lets systems detect actor death/disposal without manual bookkeeping.
- Observers should bind to these delegates rather than inventing their own tag-change events for cross-plugin tags.

## Canonical union view (`GetActorTags`)
- `GetActorTags` (and `ActorHasTag`, `ActorHasAnyTags`, `ActorHasAllTags`) is the single place to see all of an actor’s tags: interface-provided tags, the loose tags stored in `ActorLooseTags`, and the scoped counts mirrored via `AppendActorScopedTags`.
- Any cross-plugin logic that reads multiple sources should call `GetActorTags`/`ActorHasTag` so it stays aligned with what TagManager broadcasts.
- If you only have access to a world context, use `USOTS_TagLibrary::GetTagsForActor`/`AddScopedTagToActor` helpers; they forward to the subsystem so the runtime contract is preserved.
