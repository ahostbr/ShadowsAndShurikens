[CONTEXT_ANCHOR]
ID: 20250208_1540 | Plugin: SOTS_TagManager | Pass/Topic: SparseEndPlayBindingFix | Owner: Buddy
Scope: Compile fix for actor scoped tag binding + missing append declaration.

DONE
- Declared AppendActorScopedTags in subsystem header and marked HandleActorEndPlay as UFUNCTION.
- Swapped actor end play tracking to TSet<TWeakObjectPtr<const AActor>> with AddDynamic/RemoveDynamic binding (no delegate handle storage).

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Sparse dynamic delegate path via AddDynamic resolves compile errors and behaves correctly at runtime.

FILES TOUCHED
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Public/SOTS_GameplayTagManagerSubsystem.h
- Plugins/SOTS_TagManager/Source/SOTS_TagManager/Private/SOTS_GameplayTagManagerSubsystem.cpp

NEXT (Ryan)
- Rebuild to confirm TagManager compiles.
- In editor, start/stop actors to ensure end play tags clear and no duplicate bindings accumulate.

ROLLBACK
- Revert the two touched files to previous revision.