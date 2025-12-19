# Worklog — SOTS_Interaction MICRO Pickup Metadata 1 (2025-12-18 24:06)

## Goal
Populate `FSOTS_InteractionActionRequest.ItemTag` and `Quantity` for `Interaction.Verb.Pickup` so downstream routers can reliably route pickup actions to the inventory service.

## Changes
- Added data-only pickup fields to `USOTS_InteractableComponent`:
  - `FGameplayTag PickupItemTag`
  - `int32 PickupQuantity = 1`
- Updated `USOTS_InteractionSubsystem::BuildActionRequestPayload` to populate `Request.ItemTag` and `Request.Quantity` from `USOTS_InteractableComponent` when `OptionTag == Interaction.Verb.Pickup`.

## Files Touched
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractableComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Plugins/SOTS_Interaction/Docs/SOTS_Interaction_PickupMetadata.md

## Source Order
- Option payload (future)
- `USOTS_InteractableComponent` fields (added here)
- Fallback: leave `ItemTag` invalid, `Quantity=1`

## Verification
- No build/run performed (by instruction).

## Cleanup
- Deleted Plugins/SOTS_Interaction/Binaries and Plugins/SOTS_Interaction/Intermediate.

## Follow-ups
- Consider adding option-level pickup metadata if some interactables have multiple pickup options.
- Wire editor content or BPs to set `PickupItemTag` on pickup actors.
