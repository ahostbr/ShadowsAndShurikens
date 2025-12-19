# Buddy Worklog — SOTS_INV

## Goal
- Add blueprint-facing helpers in the inventory facade to fetch an inventory provider from a world context with controller-first resolution.

## What changed
- Added BlueprintPure helpers `GetInventoryProviderFromWorldContext` and `ResolveInventoryProviderFromControllerFirst` to `SOTS_InventoryFacadeLibrary`.
- Routed the internal provider resolver through the new helpers to enforce controller-first, pawn-second ordering before falling back to instigator components and the bridge subsystem.

## Files changed
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryFacadeLibrary.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp

## Notes + risks/unknowns
- Behavior is unverified; need runtime/editor validation to confirm provider resolution still matches gameplay expectations.
- Priority shift (controller before pawn/instigator) could change which provider is chosen in edge cases.

## Verification status
- Unverified (no build or runtime executed).

## Cleanup confirmation
- Plugins/SOTS_INV/Binaries (not present); Plugins/SOTS_INV/Intermediate (not present) — nothing to delete.

## Follow-ups / next steps
- Validate in editor that Blueprints can retrieve providers via the new helpers and that pickup flow resolves correctly when inventory components live on controllers/pawns.
- Run a build/editor smoke when convenient.
