[CONTEXT_ANCHOR]
ID: 20251217_1340 | Plugin: SOTS_TagManager | Pass/Topic: ScopedLooseTags | Owner: Buddy
Scope: Scoped loose tag handles, union-aware queries, events, and EndPlay cleanup wiring for loose tags.

DONE
- Added Blueprint handle type FSOTS_LooseTagHandle and scoped APIs (AddScopedTagToActor, AddScopedTagToActorByName, RemoveScopedTagByHandle).
- Added scoped-count storage + handle map with EndPlay cleanup that clears loose/scoped tags and invalidates handles; bound OnLooseTagAdded/Removed to union visibility changes.
- Updated unscoped tag add/remove to gate broadcasts on visibility and reuse union-aware queries (interface tags + unscoped + scoped counts).
- Exposed scoped helpers through SOTS_TagLibrary with world-context forwarding.

VERIFIED
- None (no build/editor/runtime executed).

UNVERIFIED / ASSUMPTIONS
- EndPlay binding covers destruction; did not add a separate OnDestroyed binding.
- OnLooseTagRemoved at EndPlay emits best-effort removals for all union-visible tags (including interface-provided tags); behavior unvalidated in runtime.
- Scoped/unscoped union semantics assumed to hold across gameplay and blueprint callers.

FILES TOUCHED
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_LooseTagHandle.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_GameplayTagManagerSubsystem.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_GameplayTagManagerSubsystem.cpp
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_TagLibrary.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_TagLibrary.cpp
- Plugins/SOTS_TagManager/Docs/Worklogs/TagManager_ScopedLooseTags_20251217_1335.md

NEXT (Ryan)
- Build/launch editor with SOTS_TagManager to ensure scoped APIs compile and are callable from Blueprint.
- Verify OnLooseTagAdded/Removed fire only on union visibility changes (interface vs scoped vs unscoped) and that handle-based removals decrement correctly.
- Destroy/EndPlay actors with active scoped/unscoped tags to confirm cleanup clears maps, invalidates handles, and emits removals once.
- Evaluate follow-up TODO list in the worklog to route direct tag queries through TagManager/TagLibrary.

ROLLBACK
- Revert the new handle file and changes to SOTS_GameplayTagManagerSubsystem.h/.cpp and SOTS_TagLibrary.h/.cpp (plus the worklog/anchor) via git revert or manual file rollback.
