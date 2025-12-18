# SOTS_UI InvSP Adapter C++ Backing

- USOTS_UIRouterSubsystem now owns the InvSP adapter instance (transient) and exposes Blueprint-callable accessors `EnsureInvSPAdapter` and `GetInvSPAdapter`.
- Optional override: `InvSPAdapterClassOverride` (BP-editable). If unset, uses config `InvSPAdapterClass` or falls back to `USOTS_InvSPAdapter`.
- Request functions exposed on the router forward into the adapter’s BlueprintNativeEvents (no InvSP hard refs):
  - `RequestInvSP_ToggleInventory`
  - `RequestInvSP_OpenInventory`
  - `RequestInvSP_CloseInventory`
  - `RequestInvSP_RefreshInventory`
  - `RequestInvSP_SetShortcutMenuVisible`
  - `RequestInvSP_NotifyPickupItem`
  - `RequestInvSP_NotifyFirstTimePickup`
- Inventory/container menu flows now invoke the adapter via `EnsureInvSPAdapter`/`GetInvSPAdapter` before pushing/popping UI.
- No Blueprint assets created; runtime wiring stays through the adapter subclass Ryan will supply.
