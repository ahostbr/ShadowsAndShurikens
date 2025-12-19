# Worklog — SOTS_INV SPINE_1 Provider Seam (code-only)

## Goal
Lock inventory item identity and expose a Blueprint provider seam plus a pickup service entrypoint without InvSP coupling.

## Changes
- Expanded `USOTS_InventoryProviderInterface` BlueprintNativeEvents: CanPickup, ExecutePickup, AddItemByTag, RemoveItemByTag, GetItemCount (instigator-aware).
- Added `ItemIdFromTag` helper (ItemId == ItemTag.GetTagName) and `RequestPickup` service entrypoint in `USOTS_InventoryFacadeLibrary`; provider resolution scans instigator/controller/pawn components or bridge provider, then calls provider interface.
- Added doc summarizing seam and identity convention.

## Files Touched
- Plugins/SOTS_INV/Source/SOTS_INV/Public/Interfaces/SOTS_InventoryProviderInterface.h
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryFacadeLibrary.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp
- Plugins/SOTS_INV/Docs/SOTS_INV_ProviderSeam.md

## Notes / Assumptions
- No builds/run; code-review stage only.
- No InvSP C++ calls or new dependencies added; provider discovery relies on the interface only.
- Existing providers must implement new BlueprintNativeEvents to participate in RequestPickup.

## Verification
- Not run (no runtime/editor validation).

## Cleanup
- Deleted Plugins/SOTS_INV/Binaries and Plugins/SOTS_INV/Intermediate (if present).
