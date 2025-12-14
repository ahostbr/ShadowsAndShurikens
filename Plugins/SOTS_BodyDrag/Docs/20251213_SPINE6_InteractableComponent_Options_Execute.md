# SOTS BodyDrag — SPINE 6 (Interactable Component & Options)

## Summary
- Added `USOTS_BodyDragInteractableComponent` implementing `ISOTS_InteractableInterface` to expose drag/drop options without Blueprint glue.
- Options resolve via TagManager (Start/Drop tags) and dispatch to the player BodyDrag component for execution.
- Supports component-based interactables via SOTS_Interaction’s component resolution.

## Option tags
- Start: `SAS.Interaction.Option.BodyDrag.Start`
- Drop:  `SAS.Interaction.Option.BodyDrag.Drop`

## How it finds the player component
- Uses interaction context instigator (player pawn first, then controller) to find `USOTS_BodyDragPlayerComponent`.
- Requires the body actor to have `USOTS_BodyDragTargetComponent`.

## Why no BP edits
- All logic lives in the native component implementing the interaction interface; SOTS_Interaction can call it directly once attached to actors.
