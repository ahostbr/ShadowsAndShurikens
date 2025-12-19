# SOTS_INV UI Bridge (SPINE_3)

- SOTS_INV never touches InvSP directly; all UI requests go through SOTS_UI `USOTS_UIRouterSubsystem`.
- Pickup notifications: on successful pickup, SOTS_INV calls `RequestInvSP_NotifyPickupItem(ItemTag, Quantity)`. First-time pickup notification hook is present but currently deferred (no first-time tracking).
- Inventory UI requests exposed by SOTS_INV forward to SOTS_UI router entrypoints:
  - `RequestInvSP_OpenInventory`
  - `RequestInvSP_CloseInventory`
  - `RequestInvSP_ToggleInventory`
  - `RequestInvSP_RefreshInventory`
  - `RequestInvSP_SetShortcutMenuVisible`
- Item identity remains `ItemId == ItemTag.GetTagName()`; `bNotifyUIOnSuccess` in pickup requests controls UI notification emission.
- UI policy/ownership stays in SOTS_UI; SOTS_INV only emits intents.
