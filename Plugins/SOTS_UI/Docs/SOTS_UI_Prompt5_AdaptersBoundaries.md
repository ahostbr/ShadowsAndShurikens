# SOTS UI Prompt 5 — Adapters & Boundaries

Rules:
- No direct ProHUD/InvSP calls outside `SOTS_UI`. Other plugins should use the router Blueprint API.
- Router owns viewport/input; adapters stay private and only forward visuals.

Router surface added (BlueprintCallable):
- `PushNotification_SOTS`, `ShowPickupNotification`, `ShowFirstTimePickupNotification`.
- `AddOrUpdateWorldMarker_Actor`, `AddOrUpdateWorldMarker_Location`, `RemoveWorldMarker`.
- `OpenInventoryMenu`, `CloseInventoryMenu`, `ToggleInventoryMenu`, `OpenItemContainerMenu`, `CloseItemContainerMenu`.
- `EnsureGameplayHUDReady` (idempotent, ensures adapters/HUD are alive).

Routing behavior:
- Notifications go to `SOTS_NotificationSubsystem` + ProHUD adapter (no CGF widgets). Pickups push registry widgets via tags (`UI.HUD.PickupNotification` / `SAS.UI.InvSP.PickupNotification` variants) and fall back to router notifications.
- World markers call waypoint subsystem and ProHUD adapter for world markers.
- Inventory menus use registry tags (`UI.Menu.Inventory` / `SAS.UI.InvSP.InventoryMenu`, `UI.Menu.Container` / `SAS.UI.InvSP.ContainerMenu`) plus InvSP adapter open/close helpers. Router owns pause/input through registry entry policies.

ProHUD feature face (current vs TODO):
- Supported now: HUD creation via adapter ensure, notifications, world markers.
- TODO (requires tag + widget mapping): objective/quest feeds, flashy pickup icon channels, settings/profiles system actions.

Tags to ensure exist:
- `UI.HUD.PickupNotification`, `UI.HUD.PickupNotification.FirstTime` (or the `SAS.UI.InvSP.*` variants).
- `UI.Menu.Inventory`, `UI.Menu.Container` (or `SAS.UI.InvSP.*` variants).

Deferred direct-call offenders (left as-is because debug-only):
- `Plugins/SOTS_Debug/.../SOTS_SuiteDebugSubsystem.cpp` (debug overlay).
- `Plugins/LightProbePlugin/.../LightLevelProbeComponent.cpp` (debug widget).
- `Plugins/SOTS_Parkour/.../SOTS_ParkourComponent.cpp` (parkour debug UI).
