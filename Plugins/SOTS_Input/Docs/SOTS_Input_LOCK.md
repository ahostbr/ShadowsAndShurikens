# SOTS_Input_LOCK (SPINE_6)

- **Scope completed (SPINE_1–6)**
  - Layered Enhanced Input routing with buffering and intent handlers.
  - Blueprint seam + stable API (`SOTS_InputAPI.h`) for other plugins.
  - Deterministic binding rebuild and buffer-aware dispatch.
  - Router lifecycle refresh, tag gates, debug/console surfaces (dev-only).
  - Async-optional layer registry with sync fallback.
  - Perf guardrails: timer-based auto-refresh (opt-in), no per-event allocations from layer list building.

- **Public API surface**
  - Include `SOTS_InputAPI.h` (stable): Blueprint library, router, layer data asset, handler base, buffer component, device/gate types.

- **Tags currently used/required**
  - Layers: `Input.Layer.Cutscene`, `Input.Layer.UI.Nav`, `Input.Layer.Dragon.Control`, `Input.Layer.Ninja.Default`.
  - Buffer: `Input.Buffer.Channel.Execution` (windows).
  - Intents: `Input.Intent.Gameplay.Interact`, `Input.Intent.UI.Back`.

- **Known limitations / TODOs**
  - Placeholder IMCs/IAs for layers still need real assets and registry entries.
  - UI ownership unchanged: SOTS_UI must drive InputMode/cursor/focus.
  - TagManager reflection gates remain optional/no-op if subsystem absent.

- **Next integration steps**
  - Wire SOTS_UI to push/pop UI.Nav using `USOTS_InputBlueprintLibrary`.
  - Author real IMCs/IAs per layer and register in SOTS Input Layer Registry.
  - Enable async registry loads only if packaging profile benefits; otherwise leave sync.
  - Use `sots.input.dump` during QA to validate layer stack/bindings/buffers.
