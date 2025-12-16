# SOTS_Input Integration Contract (SPINE_6)

- **Ownership rules**
  - SOTS_Input: mapping contexts, routing, handler buffering, buffer windows.
  - SOTS_UI: InputMode/cursor/focus and widget lifecycle (unchanged).

- **Canonical layers / priorities (desc)**
  - Input.Layer.Cutscene — Priority 1000, blocks lower.
  - Input.Layer.UI.Nav — Priority 500.
  - Input.Layer.Dragon.Control — Priority 200.
  - Input.Layer.Ninja.Default — Priority 0.

- **Push/Pop responsibilities**
  - SOTS_UI pushes `Input.Layer.UI.Nav` on menu open; pops on menu close.
  - Dragon possession/controller mode pushes `Input.Layer.Dragon.Control`; optionally pop `Input.Layer.Ninja.Default` while possessed.
  - MissionDirector/cutscenes push exclusive `Input.Layer.Cutscene` and restore prior stack on end.
  - Gameplay ensures `Input.Layer.Ninja.Default` stays present outside special modes.

- **Buffering flow**
  - Execution windows open `Input.Buffer.Channel.Execution` via `UAnimNotifyState_SOTS_InputBufferWindow`.
  - Handlers buffer when `BufferChannel` matches the open channel; router replays on close+flush.
  - Close flush preserves capture order within the channel.

- **Intent tags**
  - Required intents: `Input.Intent.Gameplay.Interact`, `Input.Intent.UI.Back` (extend via handler assets if more are needed).

- **Call surface**
  - Prefer the stable include `SOTS_InputAPI.h`; Blueprint entry points live in `USOTS_InputBlueprintLibrary`.
  - No UI InputMode/cursor/focus changes are performed in SOTS_Input.
