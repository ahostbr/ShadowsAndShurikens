# TagManager SPINE2 Union & Cleanup (2025-12-17 15:55)

What changed
- Added true union query helper (owned interface tags + unscoped loose + scoped counts) and routed ActorHasTag/Any/All (+ ByName) through it for authoritative visibility.
- Event gating now uses union visibility (pre/post diff) so OnLooseTagAdded/Removed fire only when union visibility changes.
- Scoped add/remove remains handle-based; scoped removal no longer affects unscoped tags—visibility recomputed via union.
- EndPlay backstop: bind once per actor, diff union before/after cleanup, clear unscoped/scoped maps, invalidate handles, unbind delegate, and broadcast removals for tags that actually leave the union.
- Added handle validity helper (IsScopedHandleValid) and TagLibrary wrapper for BP access.

Union semantics
- Queries see UNION of: (A) IGameplayTagAssetInterface-owned tags, (B) unscoped loose tags, (C) scoped loose tags with count > 0.

Event semantics
- OnLooseTagAdded fires when union changes from not having a tag to having it (after add/Increment).
- OnLooseTagRemoved fires when union changes from having a tag to not having it (after decrement/removal/cleanup).

EndPlay cleanup
- OnEndPlay: capture union before cleanup; remove unscoped and scoped entries; invalidate handles for that actor; unbind delegate; rebuild union after cleanup; broadcast removals for tags present before but not after.

New APIs
- Subsystem: IsScopedHandleValid (BlueprintPure)
- TagLibrary: IsScopedHandleValid (BlueprintPure, world-context)

No-bypass scan
- Report: Plugins/SOTS_TagManager/Docs/Worklogs/TagManager_NoBypassScan_20251217_1545.md (DevTools regex scan; routing targets enumerated, no refactors done).

Verification
- UNVERIFIED (no build/editor run).

Cleanup
- Plugins/SOTS_TagManager/Binaries removed.
- Plugins/SOTS_TagManager/Intermediate removed.
