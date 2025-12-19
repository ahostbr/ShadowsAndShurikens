# Worklog — SOTS_UI MICRO Pickup Fallback 1 (2025-12-18 23:59)

## Goal
Make UI routing tolerant when `FSOTS_InteractionActionRequest` is missing `ItemTag` by attempting a best-effort fallback from the target actor's `USOTS_InteractableComponent`.

## Changes
- Updated `USOTS_UIRouterSubsystem::HandleInteractionActionRequested` to inspect the request's `TargetActor` and read `PickupItemTag`/`PickupQuantity` from `USOTS_InteractableComponent` when `Request.ItemTag` is invalid.
- If fallback yields a valid `ItemTag`, the router proceeds to call `USOTS_InventoryFacadeLibrary::RequestPickup` as before.
- If fallback fails, behavior unchanged: dev-only verbose log and skip.
- Documented the fallback behavior in `SOTS_UI_InteractionActionRouting.md`.

## Files Touched
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Docs/SOTS_UI_InteractionActionRouting.md
- Plugins/SOTS_UI/Docs/Worklogs/SOTS_UI_MICRO_PickupFallback_1_20251218_2359.md

## Verification
- No builds or runtime tests performed (per instruction).

## Cleanup
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate.

## Next Steps
- Consider surfacing option-level pickup metadata so some interactables with multiple options can route distinct items.
