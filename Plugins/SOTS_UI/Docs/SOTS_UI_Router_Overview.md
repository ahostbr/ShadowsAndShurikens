# SOTS UI Router Overview

- Only the `SOTS_UI` plugin is allowed to create widgets or touch input/viewport. Other plugins talk to the router subsystem only.
- Widget registry: create a `/Game` `SOTS_WidgetRegistryDataAsset`, fill `Entries` with `WidgetId`, `WidgetClass`, `Layer`, `InputPolicy`, `Pause`, `CachePolicy`, `ZOrder`, `AllowMultiple`, `CloseOnEscape`. Point `WidgetRegistry` config on the router to this asset.
- ProHUD bridge: implement a Blueprint subclass of `USOTS_ProHUDAdapter` in `/Game` (e.g. `BP_ProHUDAdapter`) that calls into `BPI_HUDManagerV2` for notifications and world markers. Set the class on the router config; it instantiates one instance and forwards notifications/waypoints.
- Inventory bridge: optionally subclass `USOTS_InvSPAdapter` for InvSP open/close/refresh hooks; the router owns the instance when configured.
- Usage from other plugins (C++ or BP): get `SOTS_UIRouterSubsystem` and call `PushWidgetById`, `ReplaceTopWidgetById`, `PopWidget/PopAllModals`, `ShowNotification`, `AddOrUpdateWaypoint_Actor/Location`, `RemoveWaypoint`, or `SetObjectiveText`. The widget payload parameter is an `FInstancedStruct` you can pass through to your widget type for future extensibility.
