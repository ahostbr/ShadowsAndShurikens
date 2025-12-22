# Buddy Worklog — ProfileShared Snapshot Locks (20251221_235620)

Goal
- Confirm ProfileShared obeys the suite snapshot/owner rules: canonical slots, versioned payloads, deterministic providers, and transform restores that only apply valid data.

What changed
- Added `SOTS_ProfileShared_SnapshotPolicy.md` summarizing slot naming, version requirements, provider priority, the guarded transform restore, and the fact that MissionDirector still owns `TotalPlaySeconds`.
- Introduced `SnapshotVersion` on `FSOTS_ProfileSnapshot`, wired it into save/build paths, and ensured loading preserves existing values without rewriting old data.
- Hardened `RestorePlayerFromSnapshot` so transforms only apply when `ContainsNaN()` is false, avoiding invalid data from corrupt snapshots while keeping the pawn synced when valid.

Files changed
- [Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h](Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h) (version constant + field)
- [Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp](Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp) (version wiring + transform guard)
- [Plugins/SOTS_ProfileShared/Docs/SOTS_ProfileShared_SnapshotPolicy.md](Plugins/SOTS_ProfileShared/Docs/SOTS_ProfileShared_SnapshotPolicy.md)

Notes + risks/unknowns
- Snapshot version default is 1; older assets that lack the field will deserialize to 0 but continue to work. Migration hooks can check `SnapshotVersion` later.
- Provider ordering already relied on `Priority` and insertion `Sequence`, so the policy doc just codifies that deterministic behavior.
- Transform guard logs a warning when corrupt data arrives but otherwise skips the transform, leaving stats + providers untouched.

Verification status
- UNVERIFIED (no build/run).

Cleanup
- Plugins/SOTS_ProfileShared/Binaries/ deleted.
- Plugins/SOTS_ProfileShared/Intermediate/ deleted.

Follow-ups / next steps
- Ryan: ensure mission director/skill tree path still feeds `FSOTS_ProfileSnapshot.PlayerCharacter` through `ApplySnapshotToWorld`, including the new version value.
- Ryan: update any migration tooling to respect `SnapshotVersion` when backfilling older saves.