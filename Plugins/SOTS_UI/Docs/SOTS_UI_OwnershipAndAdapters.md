# SOTS UI Ownership & Adapter Surface

## Router ownership rules
- The router is the only code that pushes, replaces, or pops registry-driven widgets, caches them, and removes them before refreshing input (`PushWidgetById`/`PopWidget`/`RemoveTopFromLayer`). See `Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp#L175` and `#L214` for the stack operations and `#L733` for the single viewport add.
- `RefreshInputAndPauseState` and `ApplyInputPolicy` (lines `#L777` and `#L825`) are the exclusive call sites for `APlayerController::SetInputMode`/`SetShowMouseCursor`. `Plugins/SOTS_Input/Docs/SOTS_Input_BufferWindows.md#L42` documents that the router owns the cursor/input policy, so other modules should never call `SetInputMode` or `SetWidgetToFocus` directly.
- `UpdateUINavLayerState` toggles the `Input.Layer.UI.Nav` tag whenever the top entry is more than `GameOnly`, using `USOTS_InputBlueprintLibrary::PushLayerTag`/`PopLayerTag` (lines `#L868`, `#L882`, `#L887`). This keeps SOTS_Input navigation contexts in sync with the router stack without letting other systems poke at the nav layer directly.
- Widgets that need to emerge on-screen must go through the router so they participate in the layered Z-order, pause handling, input policies, and modal bookkeeping. Any other `CreateWidget`/`AddToViewport` (currently only `Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp#L334` and `Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp#L254`) are flagged with TODOs and should be rerouted through `USOTS_UIRouterSubsystem`.

## Back/Escape and ReturnToMainMenu flow
- System actions run through `ExecuteSystemAction`. `SAS.UI.Action.CloseTopModal` is the only action that pops the modal stack (`PopWidget(ESOTS_UILayer::Modal, true)` at `#L1236`) so any Back/Escape binding must hit that tag. `SubmitModalResult` (lines `#L602`‑`#L616`) feeds the `CancelActionTag` into `ExecuteSystemAction` and always pops the modal afterward, ensuring cancel/back buttons close the top UI.
- `RequestReturnToMainMenu` builds the stock confirm dialog (`BuildReturnToMainMenuPayload` at `#L635`) so the confirm button resolves to `SAS.UI.Action.ReturnToMainMenu` and the cancel button reruns `CloseTopModal`. When the player says “yes,” the router calls `HandleReturnToMainMenuAction` (`#L1203`) which broadcasts `OnReturnToMainMenuRequested` (`#L1205`) rather than stepping into gameplay flow. No other subsystem duplicates the `ReturnToMainMenu` intent, so the world’s travel owner can hook `OnReturnToMainMenuRequested` once and trust all UIs to use the same path.

## InvSP adapter surface
- `EnsureInvSPAdapter` (`#L516`) owns the adapter instance configured via `InvSPAdapterClass`/`InvSPAdapterClassOverride`, and every inventory interaction goes through `USOTS_InvSPAdapter` before the router touches widgets.
  * `RequestInvSP_ToggleInventory`, `RequestInvSP_OpenInventory`, `RequestInvSP_CloseInventory` (`#L546`‑`#L573`) call the adapter and then `PushInventoryWidget`/`PopWidget` so the widget stack stays coherent.
  * `RequestInvSP_RefreshInventory` (`#L562`) and `RequestInvSP_SetShortcutMenuVisible` (`#L578`) expose helper hooks for UI-driven refreshes or showing the shortcut menu while never adding widgets directly.
  * `RequestInvSP_NotifyPickupItem`/`RequestInvSP_NotifyFirstTimePickup` (`#L586`‑`#L594`) let the adapter show pickup cues without needing direct widget ownership.
  * Container flows call `OpenItemContainerMenu` and `CloseItemContainerMenu` (`#L466`, `#L487`) with container actors forwarded into `USOTS_InvSPAdapter::OpenContainer`/`CloseContainer`.

## ProHUDV2 adapter minimum scope
- `EnsureAdapters` (`#L1018`) instantiates `ProHUDAdapter` from `ProHUDAdapterClass` (falling back to the default bridge) and calls `EnsureHUDCreated` at `#L1025` and via `EnsureGameplayHUDReady` (`#L507`), so the adapter owns the ProHUDV2 root widget lifecycle.
- `ShowNotification` (`#L269`) mirrors the notification subsystem and also dispatches to `ProHUDAdapter::PushNotification`, keeping both systems in sync.
- `AddOrUpdateWaypoint`/`RemoveWaypoint` (`#L330`, `#L349`, `#L367`) are the only code paths that call `ProHUDAdapter::AddOrUpdateWorldMarker` or `RemoveWorldMarker`, giving the adapter control over on-screen markers while the router handles the underlying `SOTS_WaypointSubsystem`.
- The router seeds the registry with the `SAS.UI.ProHUDV2.*` widget IDs (see `Plugins/SOTS_UI/Docs/ProHUDV2_WidgetRegistrySeed.json`) so any ProHUDV2 notification/world-marker widgets appear without needing separate widget pushes, aligning with the “HUD lifecycle + notifications + world markers” scope described in the locked ProHUDV2 law.

## Known ownership drift
- Dev-only debug widgets in `Plugins/SOTS_Debug/...#L334` and `Plugins/LightProbePlugin/...#L254` still add themselves to the viewport; TODO comments now remind future work to route those widgets through `USOTS_UIRouterSubsystem`.
