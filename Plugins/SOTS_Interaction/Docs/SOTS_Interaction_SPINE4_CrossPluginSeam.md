# SOTS_Interaction SPINE4 – Cross-Plugin Action Seam

## Overview
- Adds a canonical action request seam at the subsystem level to route select interaction verbs to other plugins via their public entrypoints.
- Short-circuits execution before interactable interface/component calls when a canonical verb is detected.
- Routes canonical verbs to their owning subsystems (SOTS_INV, SOTS_KillExecutionManager, SOTS_BodyDrag) via public entrypoints; UI remains intent-only.

## Canonical Verbs
- Interaction.Verb.Pickup
- Interaction.Verb.Execute
- Interaction.Verb.DragStart
- Interaction.Verb.DragStop

## Hook Point
- `USOTS_InteractionSubsystem::ExecuteInteractionInternal`
  - After option validation (range/LOS/block), if `OptionTag` matches a canonical verb:
    - Build `FSOTS_InteractionActionRequest`
    - Route to the owning subsystem (pickup -> INV, execute -> KEM, drag -> BodyDrag)
    - Broadcast `OnInteractionActionRequested`
    - Return success without invoking the interactable

## Payload: FSOTS_InteractionActionRequest
- InstigatorActor (player pawn or controller)
- TargetActor
- VerbTag (option tag)
- ExecutionTag (optional; populated from InteractionTypeTag for Execute verbs when available)
- OptionIndex (or -1)
- ItemTag (best-effort; optional)
- Quantity (default 1; optional)
- ContextTags (includes InteractionTypeTag and option MetaTags when available)
- bHadLineOfSight (true if LOS check passed/was not required)
- Distance

## Driver Forwarder
- `USOTS_InteractionDriverComponent` forwards subsystem action requests via `OnActionRequestForwarded` for BP consumers.

## Pickup ItemTag/Quantity
Best-effort only. ItemTag/Quantity are filled from the target's InteractableComponent when present; otherwise the action handler skips routing.

## Constraints
- Additive changes only; no UI creation.
- Tags added to DefaultGameplayTags.ini for the canonical verbs.
