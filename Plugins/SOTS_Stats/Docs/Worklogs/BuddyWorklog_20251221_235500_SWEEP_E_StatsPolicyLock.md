# Buddy Worklog — Stats Policy Lock (20251221_235500)

Goal
- Document ownership/snapshot expectations for the stats suite so future wiring stays MD > SkillTree > Stats > UI without drift.

What changed
- Added `SOTS_Stats_OwnershipAndSnapshotPolicy.md` capturing the owner of every stat map (`USOTS_StatsComponent` on the pawn) and the helper library (`USOTS_StatsLibrary`).
- Recorded the snapshot contract around `FSOTS_CharacterStateData`/`FSOTS_ProfileSnapshot` housed in `Plugins/SOTS_ProfileShared` and emphasized `USOTS_StatsLibrary::ApplySnapshotToActor` as the canonical restore path.
- Called out integration seams: mission director milestones should still funnel through the skill tree into stats (see `TryWriteMilestoneToStatsAndProfile` in the MissionDirector docs) and UIs should observe `OnStatChanged` without adding writable caches.

Files changed
- [Plugins/SOTS_Stats/Docs/SOTS_Stats_OwnershipAndSnapshotPolicy.md](Plugins/SOTS_Stats/Docs/SOTS_Stats_OwnershipAndSnapshotPolicy.md)

Notes + risks/unknowns
- Class inventory:
  - `USOTS_StatsComponent` — `Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsComponent.h/.cpp` (stat maps, bounds, `OnStatChanged`).
  - `USOTS_StatsLibrary` — `Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsLibrary.h/.cpp` (actor lookup helpers, snapshot helpers).
  - `FSOTS_CharacterStateData` / `FSOTS_ProfileSnapshot` — `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h` (snapshot payload that Stats copy into/out of).
- Snapshot findings: stats snapshot is a simple `TMap<FGameplayTag, float>` carried through `FSOTS_CharacterStateData`; `ReadFromCharacterState` simply copies it, so restore is authoritative and there is no recompute. `BuildCharacterStateData` is used when profiles call `SnapshotActorStats`.
- Mismatch flag: there is no dedicated Stats subsystem; the policy relies on the actor component being granted to each pawn and the profile-helper functions to act as the global router. The mission director reward chain is documented but not wired in code yet—future work must keep MD > SkillTree > Stats > UI intact.

Verification status
- UNVERIFIED (no build/run).

Cleanup
- Plugins/SOTS_Stats/Binaries/ deleted.
- Plugins/SOTS_Stats/Intermediate/ deleted.

Follow-ups / next steps
- Ryan: ensure the profile snapshot loader continues to pass `FSOTS_CharacterStateData` through `USOTS_StatsLibrary::ApplySnapshotToActor` so restored stats exactly match the saved payload.
- Ryan: check MissionDirector reward paths to confirm they still mutate stats through the skill tree before emitting UI notifications.