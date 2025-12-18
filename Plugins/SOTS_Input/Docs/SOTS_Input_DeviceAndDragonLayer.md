# SOTS_Input Device Reporting and Dragon Layer (SPINE_3)

Scope: code-only seams for input device change reporting and the dragon control layer. No IMC/content wiring in this pass.

## Device change surface (PlayerController-owned)
- Device enum: `ESOTS_InputDevice` (Unknown, KBM, Gamepad) tracked by `USOTS_InputRouterComponent` (`LastDevice`).
- Router delegates: `OnInputDeviceChanged(ESOTS_InputDevice NewDevice)` broadcasts when the device flips.
- Reporting API:
  - Router: `ReportInputDeviceFromKey(FKey)` (alias of `NotifyKeyInput`).
  - Blueprint helper: `ReportInputDeviceFromKey(WorldContext, ContextActor, FKey)` uses PlayerController ensure helper to find/create the router.
  - Convenience: `GetLastDevice()`, `IsGamepadActive()`.
- Usage (recommended hook): bind an "Any Key" input on the PlayerController and call the Blueprint helper; SOTS_UI subscribes to `OnInputDeviceChanged` and owns InputMode/cursor decisions.

## Dragon control layer seam
- Tags: `Input.Layer.Dragon.Control` (existing), `Input.Layer.Ninja.Default`, `Input.Layer.UI.Nav` (existing).
- Blueprint helpers (PlayerController-aware):
  - `PushDragonControlLayer(WorldContext, ContextActor)` → pushes `Input.Layer.Dragon.Control`.
  - `PopDragonControlLayer(WorldContext, ContextActor)` → pops it.
- No mapping contexts or data assets wired in this pass; this is only the layer seam.

## Notes
- Follows IN-02: router/buffer live on PlayerController; helpers resolve/ensure there.
- Follows IN-04: router reports device changes; UI decides policy.
- No builds/run performed for this pass.
