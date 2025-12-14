# Component-Based Interactables — SPINE 2 (2025-12-13)

## Summary
- Interaction subsystem now resolves interactable implementers on actors **or** their components (actor priority).
- Interface calls (CanInteract/GetInteractionOptions/ExecuteInteraction) now dispatch to the resolved implementer instead of assuming the actor implements the interface.

## Why
- Needed for BodyDrag and future behaviors that live purely on components attached to actors (Blueprint-only project, no base character dependency).

## Resolution priority
1) If the target actor implements `USOTS_InteractableInterface`, use the actor.
2) Otherwise, use the first component on that actor whose class implements `USOTS_InteractableInterface`.
3) If none implement, interaction interface calls are skipped.

## Notes
- CurrentTarget remains the actor for display/name; only interface calls switch to the resolved implementer.
