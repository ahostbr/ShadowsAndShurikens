# TagManager MICRO PASS — GetActorTags (20251218_0005)

Goal
- Confirm canonical union tag view exists and is exposed via subsystem + tag library.

Verified / changed
- Subsystem `GetActorTags(const AActor* Actor, FGameplayTagContainer& OutTags) const` is now BlueprintCallable; continues to build union of owned (IGameplayTagAssetInterface), TagManager loose tags, and scoped/handle tags.
- TagLibrary now exposes `GetActorTags(const UObject* WorldContextObject, AActor* Target, FGameplayTagContainer& OutTags)` forwarding to the subsystem (resets OutTags, then union).
- Added TagBoundary doc note that `GetActorTags` is the canonical union view API.

Union semantics (unchanged)
- Owned tags via IGameplayTagAssetInterface
- Loose tags from TagManager `ActorLooseTags` storage
- Scoped tags from `ActorScopedTagCounts` (count > 0)

Notes
- No DefaultGameplayTags.ini edits.
- No build/run performed.

Cleanup
- Plugins/SOTS_TagManager/Binaries/ deleted.
- Plugins/SOTS_TagManager/Intermediate/ deleted.
