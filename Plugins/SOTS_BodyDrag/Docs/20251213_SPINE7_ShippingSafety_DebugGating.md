# SOTS BodyDrag — SPINE 7 (Shipping Safety & Debug Gating)

## Debug gating
- CVar `SOTS.BodyDrag.Debug` (default 0) controls debug draws; compiled out in Shipping/Test.
- When enabled and dragging: draws line to body and a string with DragState + BodyType.

## Failure/safety handling
- Mesh/anim missing: begin/drop still advance state; montage may be skipped but state continues.
- Null montages: fall through to state transitions (Entering->Dragging, Exiting->None) without crashing.
- Body destroyed mid-drag: drop finalizes safely, restores speed/tags and clears references.
- Missing components/config: drop gracefully finalizes without leaving state stuck.

## Notes
- No build executed; debug is non-config and only active in non-shipping builds.
