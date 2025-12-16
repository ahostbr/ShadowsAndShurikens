# SOTS_Input Overview (SPINE_6)

Scope: runtime-only Enhanced Input routing and buffering without GAS or UI focus changes.

## Components
- **USOTS_InputRouterComponent**: owns the active layer stack, applies mapping contexts in priority order, dispatches input to runtime handler instances, and respects blocking layers. Buffered events flush back through the same routing path. Uses owned binding indices for non-destructive Enhanced Input binding removal. Deterministic binding rebuild: unique (Action, TriggerEvent) pairs are sorted by action name then trigger enum, and bound in that order (contexts applied up to the first blocking layer). Dispatch list is precomputed per rebuild to avoid per-event allocations. Optional debug logging of active layers/device/buffer top; router tick is off by default (only enabled for debug logging).
- **USOTS_InputLayerDataAsset**: config for a layer (layer tag, priority, block flag, mapping contexts, handler templates). Templates are duplicated at runtime when the layer is pushed.
- **USOTS_InputHandler**: per-action/per-event handling unit. Supports buffering via `bAllowBuffering` + `BufferChannel` and exposes `HandleInput`/`HandleBufferedInput` plus activation hooks.
- **USOTS_InputBufferComponent**: tracks open buffer channels and holds buffered events until the channel closes. Flush uses the router to redispatch.
- **UAnimNotifyState_SOTS_InputBufferWindow**: opens a buffer channel on `NotifyBegin` and closes (optionally flushing) on `NotifyEnd`.
- **Optional Tag Gating**: router can query `USOTS_GameplayTagManagerSubsystem` (reflection to `ActorHasTag`) using `GateRules` to allow/deny buffering or live dispatch. If the subsystem/class is absent, gating is a no-op.
- **Consume Policy**: each layer can choose `None`, `ConsumeHandled`, or `ConsumeAllMatches` to control how input propagation stops within the stack (independent of `bBlocksLowerPriorityLayers`).
- **Intent Broadcast**: `OnInputIntent` delegate and `USOTS_InputHandler_IntentTag` broadcast intents from both live and buffered inputs. Interact/MenuBack handler subclasses set default intent tags.

## Layer registry
- Configure `SOTS Input Layer Registry` (Project Settings → Plugins) to map `LayerTag` → `USOTS_InputLayerDataAsset` soft refs.
- At runtime `USOTS_InputLayerRegistrySubsystem` loads these entries. Supports synchronous lookup or optional async loading (opt-in in settings); async mode requests assets via StreamableManager, then the next call to `PushLayerByTag` will succeed once loaded.

## PushLayerByTag API
- Call `PushLayerByTag(LayerTag)` on the router; it resolves via the registry subsystem and then calls `PushLayer`.
- `PopLayerByTag`, `ClearAllLayers`, `IsLayerActive` remain available for stack maintenance.

## Integration seam
- Use `USOTS_InputBlueprintLibrary` to get/push/pop: `GetRouterFromActor`, `PushLayerTag`, `PopLayerTag`, `OpenBuffer`, `CloseBuffer`. ContextActor can be controller, pawn, or any actor (falls back to first player controller). This is the surface SOTS_UI and other plugins should call.
- Stable include for consumers: `SOTS_InputAPI.h`.
- Integration rules live in `SOTS_Input_IntegrationContract.md`; checkpoint in `SOTS_Input_LOCK.md`.

## Usage sketch
1) Add `SOTS_InputTags.ini` to project config (already in plugin Config/Tags). Ensure `SAS.Input.Buffer.*` and `SAS.Input.Layer.*` tags exist in TagManager.
2) Place `USOTS_InputRouterComponent` and `USOTS_InputBufferComponent` on the pawn/actor receiving input. Router auto-resolves `EnhancedInputComponent` (on owner or controller) and the local player subsystem.
3) Author `USOTS_InputLayerDataAsset` assets: assign a layer gameplay tag, priority, mapping contexts, handler templates, and blocking preference. Handlers declare actions, trigger events, buffering permissions, and channels.
4) At runtime, push/pop layers via the router (`PushLayer`, `PopLayerByTag`, `ClearAllLayers`, `IsLayerActive`). The highest-priority layers apply until a blocking layer is encountered.
5) Use buffering when desired: open a channel via code or `AnimNotifyState_SOTS_InputBufferWindow`, let handlers with matching `BufferChannel` capture events, then close+flush to replay through handlers.

## Golden path usage (suggested flow)
- Game start: push `Input.Layer.Ninja.Default` (gameplay default).
- Open menus/inventory: push `Input.Layer.UI.Nav` (UI navigational contexts); SOTS_UI will still own input mode/cursor/focus.
- Enter cutscene: push `Input.Layer.Cutscene` with block flag to freeze lower layers; pop when done.
- SOTS_UI, dragon possession, or mission director should call the BlueprintLibrary seam to push/pop as needed.

## Device detection hook
- Use `NotifyKeyInput(FKey)` on the router (e.g., from an “Any Key” binding) to update `LastDevice` and broadcast `OnInputDeviceChanged` (KBM vs Gamepad). No UI/input mode changes are performed here.

## Intent broadcasting handler
- `USOTS_InputHandler_IntentTag` maps (InputAction, TriggerEvent) → `FGameplayTag` intent and broadcasts via `OnInputIntent` on the router for both live and buffered inputs.
- Concrete helpers: `USOTS_InputHandler_Interact` (Intent = `Input.Intent.Gameplay.Interact`) and `USOTS_InputHandler_MenuBack` (Intent = `Input.Intent.UI.Back`). Drop these into layer HandlerTemplates.

## Tag gates
- Enable/disable gating via router settings. `GateRules` require/forbid tags; each rule can apply to buffering and/or live handling.
- Reflection target: `USOTS_GameplayTagManagerSubsystem` and its `ActorHasTag` function. If missing, gates are skipped.

## Buffer channel stack
- Buffer component keeps a stack of open channels. Opening a channel moves it to the top; closing removes it. The router uses the top channel for buffering decisions.

## Debug surface
- Optional logging: set `bDebugLogRouterState` and `DebugLogIntervalSeconds` on the router to periodically log active layers, last device, and top buffer channel. Logging is compiled out of Shipping/Test; binding list logging is gated by `bDebugLogBindings` (also compile-guarded).
- BP getters: `GetLastDevice`, `GetActiveLayerTags`, `GetOpenBufferChannels`, `GetTopBufferChannel`.
- Dev console commands (non-Shipping/Test):
	- `sots.input.dump`: prints active layers (tag/priority/block/consume, contexts/handlers), buffer stack, and deterministic binding list.
	- `sots.input.refresh`: forces the first local router to re-resolve and rebuild.

## Lifecycle resilience
- `RefreshRouter()` re-resolves the owning PC, Enhanced Input Component, and local player subsystem, clears/re-applies contexts for active layers (up to first blocking layer), and rebuilds deterministic bindings. Tick is only for debug logging.
- Auto-refresh is opt-in (`bEnableAutoRefresh`) and uses a timer (default 0.5s) instead of per-frame polling.
- `EndPlay` clears contexts, router-owned bindings, buffer channels, and deactivates handlers.

## Layer consume policy
- `None`: behaves like before—only `bBlocksLowerPriorityLayers` affects propagation.
- `ConsumeHandled`: stop routing once this layer handles input.
- `ConsumeAllMatches`: evaluate this layer then stop routing regardless of handled state.

## Example layer assets (placeholders)
- Added placeholder assets under `Plugins/SOTS_Input/Content/InputLayers/` for NinjaDefault, UINav, and Cutscene layer tags. Open and re-save them in-editor as `USOTS_InputLayerDataAsset`, then register in Project Settings → SOTS Input Layer Registry.
- Recommended HandlerTemplates once opened: NinjaDefault → add `SOTS_InputHandler_Interact`; UINav → add `SOTS_InputHandler_MenuBack`; Cutscene → keep empty/blocking.

## Notes
- No gameplay ability system or UI input mode changes are performed here.
- Router binds only the trigger events declared on handler templates, minimizing redundant bindings.
- Buffered replay uses value-only dispatch (no synthetic `FInputActionInstance` for buffered events).
- Flush order preserves capture order within a channel; cross-channel ordering is not guaranteed.
- Pattern reference only: see `Plugins/NinjaInpaf825c9494a8V5/` for ideas; do not add it as a dependency.
- Build was not run while authoring SPINE_6.
