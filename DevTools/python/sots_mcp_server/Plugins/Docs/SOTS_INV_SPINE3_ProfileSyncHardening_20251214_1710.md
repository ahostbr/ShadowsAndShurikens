# SOTS_INV — SPINE 3 (Profile Sync Hardening)

## Summary
- Added snapshot validation, deterministic build/apply flow, and optional deferred apply when provider isn’t ready (off by default). Defaults preserve existing behavior; validation/deferred are opt-in.

## Snapshot contract
- Uses existing `FSOTS_InventoryProfileData` (CarriedItems, StashItems, QuickSlots).
- Validation checks (warnings/errors): invalid/None ItemId, non-positive quantity, duplicate ItemIds, quickslot slot index <0, quickslot referencing ItemId not present in items.
- Config flags (default false): `bValidateInventorySnapshotOnBuild`, `bValidateInventorySnapshotOnApply`, `bDebugLogSnapshotValidation`, `bEnableDeferredProfileApply`, `DeferredApplyMaxRetries=20`, `DeferredApplyRetryIntervalSeconds=0.5`.

## Build/apply behavior
- `BuildProfileData` optionally validates snapshot (dev-only logging). Order unchanged: read carried/stash/quickslots.
- `ApplyProfileData` now:
  - Optionally validates snapshot; errors abort apply (when enabled).
  - If provider not ready and `bEnableDeferredProfileApply` is true, queues snapshot and retries on a timer until ready or max retries, then applies in order: clear carried/stash → add carried → add stash → apply quickslots. Caches updated post-apply.
- Deferred apply is OFF by default; no behavior change unless enabled.

## Evidence pointers
- Validation + deferred apply: `ValidateInventorySnapshot`, `TryApplyPendingSnapshot` in `Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryBridgeSubsystem.cpp`.
- Config flags/pending state: `bValidateInventorySnapshotOnBuild/Apply`, `bEnableDeferredProfileApply`, retry settings, pending snapshot fields in `Public/SOTS_InventoryBridgeSubsystem.h`.
- Apply/build flow changes: `BuildProfileData`, `ApplyProfileData` in `Private/SOTS_InventoryBridgeSubsystem.cpp`.

## Behavior impact
- Defaults mirror previous behavior; validation and deferred apply require opt-in. Apply order is explicit and deterministic when run.
