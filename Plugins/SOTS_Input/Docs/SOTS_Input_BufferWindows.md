# SOTS_Input Buffer Windows (IN-03)

Scope: minimal montage buffer windows for execution/vanish/QTE flows. Queue size = 1 per channel; latest wins; auto-clear on montage end/cancel/abort.

## Rules
- Supported channels: `Input.Buffer.Channel.Execution`, `Input.Buffer.Channel.Vanish` (hard-coded allowlist).
- Windows must be explicitly opened/closed; one buffered intent per channel (latest overwrites).
- Auto-clear fires on montage end/cancel/abort via AnimInstance delegates when any window is open.
- No auto-consume on close; gameplay decides when to consume.

## API (USOTS_InputBufferComponent)
- `OpenBufferWindow(ChannelTag)`: opens a window for allowed channels; binds montage delegates on first open.
- `CloseBufferWindow(ChannelTag)`: closes the window; keeps buffered intent (if any); unbinds when none remain.
- `TryBufferIntent(ChannelTag, IntentTag, out bBuffered)`: if the window is open and channel is allowed, stores intent (latest wins, timestamped) and reports success.
- `ConsumeBufferedIntent(ChannelTag, out IntentTag, out bHadBuffered)`: returns/removes buffered intent if present.
- `ClearChannel(ChannelTag)`: closes window and clears buffered intent.
- `ClearAllBufferWindowsAndInputs()`: clears everything and unbinds montage delegates.

Legacy buffer APIs (channels, stacks, and buffered input events) remain unchanged.

## AnimNotifyState: UAnimNotifyState_SOTS_InputBufferWindow
- Properties: `BufferChannel` (default `Input.Buffer.Channel.Execution`), optional `AllowedIntentTags` (informational), `bFlushOnEnd` (ignored for windowed path).
- NotifyBegin: resolves/ensures buffer on the owning PlayerController and opens the buffer window.
- NotifyEnd: closes the buffer window (no auto-consume).

Usage:
1) Add the notify state to montage sections that should accept a buffered intent.
2) Set `BufferChannel` to Execution or Vanish/QTE as appropriate.
3) Gameplay/UI can call `ConsumeBufferedIntent` after montage windows to act on the latest buffered intent.

## Router seam (intent buffering)
- `USOTS_InputHandler_IntentTag` now attempts to buffer the intent tag into Execution/Vanish channels when handling live input; dispatch continues normally afterward.
- Buffering respects channel allowlist and window-open state; queue size remains 1 (latest wins).

## Router ownership & PC spine
- Router and buffer components are intended to be installed on the owning PlayerController. `EnsureRouterOnPlayerController`/`EnsureBufferOnPlayerController` in `SOTS_InputBlueprintLibrary` resolve the context actor to the PC (or its controller) and transiently add the component there, so all gameplay and UI callers tap the same runtime spine rather than spawning per-actor components.
- `USOTS_InputRouterComponent::TryResolveOwnerAndSubsystems` caches the `OwningPC`, `EnhancedInputComponent`, `InputSubsystem`, and `BufferComp` from the PlayerController, which keeps the router tied to that controller and ensures buffered state is shared by the PC’s singleton buffer component.
- Channel windows use the hard-coded allowlist in `IsChannelAllowedForWindow` and the `OpenWindows`/`BufferedIntentByChannel` tracking maps, guaranteeing one buffered intent slot per channel (queue size = 1, latest overwrites) while a window is open.

## Device change & UI mode
- The router exposes `FSOTS_OnInputDeviceChanged` and updates `LastDevice` whenever `NotifyKeyInput` identifies a new `ESOTS_InputDevice` via `SOTS_InputDeviceLibrary`. Blueprint helpers such as `ReportInputDeviceFromKey` forward key reports through the PlayerController’s router so UI layers always hear the PC’s definitive state.
- SOTS_UI owns the cursor/input-mode policy; `USOTS_UIRouterSubsystem::ApplyInputPolicy` is the only code that invokes `APlayerController::SetInputMode` for UI stacks (switching between `FInputModeGameOnly`, `FInputModeUIOnly`, and `FInputModeGameAndUI`). Device-change events can subscribe to the router’s delegate, but the actual input-mode transitions live entirely inside SOTS_UI rather than within SOTS_Input.

## Notes
- No builds run for this documentation pass.
- Tag additions (if any) live in Config/Tags/SOTS_InputTags.ini.
