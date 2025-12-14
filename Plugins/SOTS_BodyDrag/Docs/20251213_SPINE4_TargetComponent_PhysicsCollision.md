# SOTS BodyDrag — SPINE 4 (Target Component Physics/Collision)

## Summary
- Added `USOTS_BodyDragTargetComponent` to make actors draggable via component-only setup.
- Component resolves the body mesh (override or first skeletal mesh), caches collision/physics, disables them during drag, and restores on drop.
- Blueprint events exposed for drag start/end notifications.

## Cached/restored
- Collision profile name and collision enabled state.
- Simulate Physics flag (restored if previously true and caller opts to drop to physics).

## KO vs Dead recommendation
- Dead bodies: call `EndDragged(true)` to re-enable physics on drop.
- Knocked-out bodies: call `EndDragged(false)` to keep physics off on drop.
