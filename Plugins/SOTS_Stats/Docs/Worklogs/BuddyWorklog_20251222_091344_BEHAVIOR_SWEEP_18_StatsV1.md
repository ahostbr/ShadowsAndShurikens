# Buddy Worklog - BEHAVIOR_SWEEP_18_StatsV1 (20251222_091344)

Goal
- Solidify Stats v1: authoritative snapshot apply, per-pawn components, and stat change events consumable by MD/SkillTree/UI.

What changed
- Snapshot apply is now deterministic and eventful: `ReadFromCharacterState` and `ApplyCharacterStateData` clear then apply incoming payloads via `InternalSetStat` so `OnStatChanged` fires per stat (clamped to bounds). Added `ApplyCharacterStateData_Quiet` for cases that need silent restore.
- Library now resolve-or-creates a stats component when applying or mutating stats (`ResolveOrAddStatsComponent` + `ResolveStatsComponentForActor`), ensuring each pawn gets its own component without cross-contamination.
- SetActorStat/AddToActorStat and ApplySnapshotToActor now auto-create a component if missing, keeping the per-actor isolation intact.

Files changed
- Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsComponent.h
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsComponent.cpp
- Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsLibrary.h
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsLibrary.cpp

Notes / Decisions
- Authoritative snapshot apply overwrites and broadcasts changes; use `_Quiet` when a non-broadcast apply is desired (e.g., preload).
- Per-pawn isolation preserved: library helpers auto-attach a component to the target actor instead of sharing state elsewhere.

Verification
- Not run (Token Guard; no builds/tests).

Cleanup
- Deleted Plugins/SOTS_Stats/Binaries and Plugins/SOTS_Stats/Intermediate.
