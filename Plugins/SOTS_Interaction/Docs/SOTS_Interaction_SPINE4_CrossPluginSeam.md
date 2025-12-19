# SOTS_Interaction SPINE4 – Cross-Plugin Action Seam

## Overview
- Adds a canonical action request seam at the subsystem level to route select interaction verbs to other plugins without direct dependencies.
- Short-circuits execution before interactable interface/component calls when a canonical verb is detected.
- No UI creation changes and no direct calls to SOTS_INV / KEM / BodyDrag.

## Canonical Verbs
- Interaction.Verb.Pickup
- Interaction.Verb.Execute
- Interaction.Verb.DragStart
- Interaction.Verb.DragStop

## Hook Point
- `USOTS_InteractionSubsystem::ExecuteInteractionInternal`
  - After option validation (range/LOS/block), if `OptionTag` matches a canonical verb:
    - Build `FSOTS_InteractionActionRequest`
    - Broadcast `OnInteractionActionRequested`
    - Return success without invoking the interactable

## Payload: FSOTS_InteractionActionRequest
- InstigatorActor (player pawn or controller)
- TargetActor
- VerbTag (option tag)
- OptionIndex (or -1)
- ItemTag (best-effort; currently unused)
- Quantity (default 1)
- ContextTags (includes InteractionTypeTag when available)
- bHadLineOfSight (true if LOS check passed/was not required)
- Distance

## Driver Forwarder
- `USOTS_InteractionDriverComponent` forwards subsystem action requests via `OnActionRequestForwarded` for BP consumers.

## Pickup ItemTag/Quantity
- Best-effort only; no pickup metadata is currently surfaced on interaction options/components.
- Downstream adapters should tolerate missing ItemTag/Quantity until a pickup payload source is added.

## Constraints
- Additive changes only; no new plugin dependencies or UI creation.
- Tags added to DefaultGameplayTags.ini for the canonical verbs.
