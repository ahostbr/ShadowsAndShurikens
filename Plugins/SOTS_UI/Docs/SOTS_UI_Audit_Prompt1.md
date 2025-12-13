# SOTS UI Audit — Prompt 1

## Summary
- Core router/API lives in SOTS_UI; registry-driven stack is already present with clear layer ordering (HUD < Overlay < Debug < Modal). Debug sits above HUD but modal is topmost.
- Registry loading is config-driven via `SOTS_UISettings` (project settings entry) with adapters for ProHUD and InvSP; defaults resolve if not set on the subsystem.
- CGF exported menus show four top-level screens (Main, Pause, Settings, Loading) that should be registry-driven; AlwaysAvailable stack uses a gameplay tag guard but the exact array contents are not fully readable from the export.
- InvSP exported widgets include all requested top-level menus and pickup/shortcut HUD pieces; the gameplay-tag table export is empty so tagging-based routing is unclear.
- ProHUDV2 export centers on `WB_ProHUDV2` + `WB_HUD_Container` for compass/minimap/world markers/notifications; no extra container requirements surfaced beyond the existing set.
- Non-SOTS_UI viewport/input callers are limited to parkour stats overlay, debug suite overlay, and LightProbe debug widget.

## Registry-driven widget candidates (CGF menus)
- /Game/CoreGameFramework/UI/Widgets/Menus/BPW_MainMenu (top-level screen; sets focus and GI references; no child creation observed beyond internal content widget)
- /Game/CoreGameFramework/UI/Widgets/Menus/BPW_CGF_PauseMenu (top-level pause screen; handles input/focus; responds to BPI UI events)
- /Game/CoreGameFramework/UI/Widgets/Menus/BPW_CGF_SettingsMenu (top-level settings screen; complex internal slot construction)
- /Game/CoreGameFramework/UI/Widgets/Menus/BPW_CGF_LoadingScreen (top-level loading screen)
- CGF AlwaysAvailable stack: variable `AlwaysAvailable_WidgetStack` exists in BP_CGF_HUD and is iterated for push; a gameplay tag default of `UI.AlwaysAvailableWidgets.SettingsMenu` appears, but the export does not expose the concrete class list. Needs confirmation from in-editor defaults.

## InvSP top-level widgets + tagging check
- Top-level menus (ExampleContent/Horror/UI/Menus):
  - /Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror
  - /Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_ItemContainerMenu_Horror
  - /Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_ShopVendorMenu_Horror (exists; not in required list but present)
- HUD/notifications/shortcut widgets (ExampleContent/Horror/UI/Widgets):
  - Shortcut HUD: WBP_ItemShortcutCross_Horror
  - Pickup notifications: WBP_PickupNotification_Horror, WBP_FirstTimePickupNotification_Horror
  - Other internal children: WBP_ContextMenu_Horror, WBP_DragItem_Horror, WBP_InteractionPopup_Horror, WBP_Slot_Horror, WBP_SurvivalInfo_Horror, etc.
- InvSP UI tagging system: the exported data table DT_InventoryTags.json is empty; no textual evidence of gameplay-tag based routing found in the export. Tag usage for UI open/close remains unconfirmed.

## ProHUDV2 required top-level list
- Root: /Game/SOTS/ThirdParty/Systems/ProHUDV2/Widgets/WB_ProHUDV2 (wraps HUD container and exposes compass/minimap/world-marker + notification entrypoints).
- Core container: /Game/SOTS/ThirdParty/Systems/ProHUDV2/Widgets/Content/WB_HUD_Container (referenced throughout WB_ProHUDV2 for compass, minimap, world markers, mission/option/update notifications).
- Notification/marker flows observed: mission notifications, update/option notifications, compass & minimap markers, world markers. No additional container widgets beyond HUD container surfaced; crosshair/UI cosmetics not included here by design.

## Non-SOTS_UI viewport/input callers (C++)
- [Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L4479-L4492](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L4479-L4492) — creates/viewport-adds parkour stats widget (likely debug/telemetry overlay).
- [Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp#L256-L277](Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp#L256-L277) — debug KEM anchor overlay creation, adds to viewport.
- [Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp#L215-L236](Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp#L215-L236) — optional debug widget gated by `bShowDebugWidget`, adds to viewport with high Z-order.

## SOTS_UI API surface notes
- Router entrypoints: PushWidgetById, ReplaceTopWidgetById, PopWidget (optional layer filter), PopAllModals, SetObjectiveText, ShowNotification, waypoint add/update/remove helpers.
- Registry loading: `USOTS_UIRouterSubsystem` consumes `WidgetRegistry` (config), falling back to `USOTS_UISettings::DefaultWidgetRegistry` if unset. Adapters (`ProHUDAdapterClass`, `InvSPAdapterClass`) also default from settings. Registry asset type: `USOTS_WidgetRegistryDataAsset` containing `FSOTS_WidgetRegistryEntry` rows.
- Layer ordering / base ZOrder: HUD=0, Overlay=100, Debug=1000, Modal=2000. Layer priority for input/pause traversal: Modal > Overlay > Debug > HUD.
- Registry entry fields: WidgetId (GameplayTag), WidgetClass (soft class), Layer, InputPolicy (GameOnly/UIOnly/GameAndUI), bPauseGame, CachePolicy (Recreate/KeepAlive), ZOrder offset, bAllowMultiple, bCloseOnEscape.
- Adapters: `USOTS_ProHUDAdapter` (EnsureHUDCreated, PushNotification, AddOrUpdateWorldMarker, RemoveWorldMarker) and `USOTS_InvSPAdapter` (OpenInventory, CloseInventory, RefreshInventoryView) are BlueprintNativeEvent bridges. Ability library: `USOTS_UIAbilityLibrary::NotifyAbilityEvent` BlueprintCallable helper.

## Prompt 2 prerequisites / data needed
- Confirm the concrete `AlwaysAvailable_WidgetStack` entries inside BP_CGF_HUD (current export obscures the array values).
- Enumerate the active widget registry asset currently referenced by `SOTS_UISettings.DefaultWidgetRegistry` and list its entries (IDs, classes, layers, cache/input policies, ZOrder).
- Verify whether InvSP actually uses gameplay tags for UI routing (e.g., in Blueprint graphs or a populated DT_InventoryTags) and capture the tag IDs if so.
- Identify ProHUD container classes required by WB_ProHUDV2 beyond `WB_HUD_Container` (if any) by inspecting the live asset hierarchy in-editor.
