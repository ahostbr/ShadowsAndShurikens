# SOTS ProfileShared Snapshot Policy

## Canonical save folders
- The subsystem always saves to `SOTS_Profile_<SlotIndex>_<ProfileName>` (`GetSlotNameForProfile`). Keep tooling aligned with that pattern; no alternate folder names are defined and switching would leave legacy saves untested.
- Slot names are user-index 0 only; future multi-user support must extend the slot naming helper rather than overwrite the existing format.

## Snapshot versioning
- `FSOTS_ProfileSnapshot` now carries `SnapshotVersion`, defaulting to `SOTS_PROFILE_SHARED_CURRENT_SNAPSHOT_VERSION` (currently 1). This field travels with every serialized snapshot so future migrations can detect and upgrade older payloads.
- Building or saving a snapshot writes the current version, while loading retains the saved value; missing versions (older snapshots) simply report `0` and continue to load without corruption.

## Provider arbitration
- Providers register via `RegisterProvider` and supply a `Priority`. The registry sorts by `Priority` (higher first) and then by registration `Sequence`, so build/apply ordering is deterministic even when priorities tie.
- Each provider implements `ISOTS_ProfileSnapshotProvider` and receives every snapshot both during `BuildSnapshotFromWorld` (after gathering player data) and `ApplySnapshotToWorld` (after the pawn transform/stat restore), so hooks can decide whether to mutate or read the snapshot.
- Provider arbitration caches the highest-priority provider and emits a one-time verbose log when multiple providers are registered and the winner changes.

## Transform restore policy
- `ApplySnapshotToWorld` always honors the saved transform when it is valid: the code now guards `Transform.ContainsNaN()` and logs if a corrupt transform arrives.
- Invalid transforms are skipped to prevent the snapshot from warping the pawn, but valid transforms always overwrite the current pawn transform so replaying a snapshot reproduces the authoritative pose.

## Total play seconds ownership
- This plugin records `FSOTS_ProfileMetadata.TotalPlaySeconds` but does not increment it. The mission director owns `TotalPlaySeconds` (see the mission dedication rules in `SOTS_Suite_ExtendedMemory_LAWs.txt`), so ProfileShared never mutates it.
- `BuildSnapshotFromWorld` queries `USOTS_MissionDirectorSubsystem::GetTotalPlaySeconds` (if available) and writes the result into `Meta.TotalPlaySeconds`.

## Restore sequencing
- `ApplySnapshotToWorld` order: transform restore, stats restore, provider apply (MissionDirector, SkillTree, etc.), then `OnProfileRestored` broadcast.

## Expected behavior matrix
| Operation | Contract |
| --- | --- |
| SaveProfile | writes every field (Meta, PlayerCharacter, SnapshotVersion) into `USOTS_ProfileSaveGame` and uses the canonical slot name.
| LoadProfile | loads from the same slot, preserves the stored `SnapshotVersion`, and reuses the saved display name when present.
| BuildSnapshotFromWorld | rewrites `PlayerCharacter` transform/stat data, sets the snapshot version to current, and still calls all registered providers for their slices.
| ApplySnapshotToWorld | restores pawn transform if valid, applies optional stats via `USOTS_StatsComponent`, and then runs providers in priority order.
| Provider register/unregister | `Priority` + `Sequence` provide deterministic order and automatically drop invalid/destroyed providers via `PruneInvalidProviders`.

## Known limitations
- Snapshot migrations must be implemented in callers (ProfileShared exposes the version field but does not perform automatic upgrades). Upgrades should inspect `SnapshotVersion` before calling into other plugins.
- Multi-user save slots still assume user index 0; extending to multiple slots must respect the naming helper so current saves remain accessible.
