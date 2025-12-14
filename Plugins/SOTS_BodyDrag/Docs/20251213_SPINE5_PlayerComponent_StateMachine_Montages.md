# SOTS BodyDrag — SPINE 5 (Player Component State/Montages)

## State diagram
None -> Entering -> Dragging -> Exiting -> None
- Entering: start montage plays, attach + physics/collision disabled on body component.
- Dragging: tags applied; walk speed overridden.
- Exiting: stop montage plays; detach, restore physics/collision, tags, and walk speed.

## Tags
- Applied on start montage end: `DraggingStateTagName` + `DraggingDeadTagName` or `DraggingKOTagName` based on body type.
- Removed after drop finalization.

## AnimBP usage
- Read `DragState` (ESOTS_BodyDragState) to drive pose graph blending.
- Anim montages drive transitions; interruptions still advance the state machine to avoid getting stuck.

## Notes
- Component assumes owner is ACharacter for mesh + movement; no project CharacterBase dependency.
- Body attaches to the character mesh at `AttachSocketName` when configured; keep-world detach on drop.
- Physics re-enable controlled by config (Dead vs KO) via target component.
