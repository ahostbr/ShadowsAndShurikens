# SOTS_UI Prompt 3 — Debug Layer ZOrder

Changes:
- `GetLayerBaseZOrder` now makes the Debug layer the highest visual layer (HUD=0, Overlay=100, Modal=1000, Debug=10000) while leaving input priority untouched.
- Added a startup log in `USOTS_UIRouterSubsystem::Initialize` to print base ZOrders once for verification.

Behavior:
- Visual ordering is governed solely by `GetLayerBaseZOrder`; input focus still follows `LayerPriority` (Modal first, then Overlay, Debug, HUD).
- Existing per-entry `ZOrder` offsets still apply on top of the base values.

Before/After:
- Before: HUD=0, Overlay=100, Debug=1000, Modal=2000 (Debug could be under Modal).
- After:  HUD=0, Overlay=100, Modal=1000, Debug=10000 (Debug always on top visually).
