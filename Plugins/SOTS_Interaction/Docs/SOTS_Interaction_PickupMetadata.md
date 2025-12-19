# SOTS_Interaction — Pickup Metadata (Micro Pass 1)

## Summary
- Canonical pickup metadata source order for `FSOTS_InteractionActionRequest` when `Verb == Interaction.Verb.Pickup`:
  1. Option payload (if option struct is extended in future)
  2. `USOTS_InteractableComponent` fields `PickupItemTag` and `PickupQuantity` (added in this pass)
  3. Fallback: no `ItemTag`, `Quantity = 1` (caller must tolerate)

## Rationale
- Keeps interaction subsystem decoupled from inventory; downstream routers (SOTS_UI) can always attempt pickup routing using the action request payload.
- Minimal, additive data-only change to `USOTS_InteractableComponent` to surface pickup identity and quantity.

## Files Touched
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractableComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Plugins/SOTS_Interaction/Docs/Worklogs/SOTS_Interaction_MICRO_PickupMetadata_1_<timestamp>.md

## Notes
- Option payload support is left for a future pass; current pragmatic source is the interactable component.
- No gameplay logic added to the component; it's data-only and editable in editor/BPs.
