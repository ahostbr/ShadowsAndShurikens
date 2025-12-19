# SOTS_FX_Plugin — Planned-Behaviors Completeness Sweep (PLAN)

## Current inventory (Phase 0 evidence)
- **Subsystem**: `USOTS_FXManagerSubsystem` with singleton accessor and cue registration (`Public/SOTS_FXManagerSubsystem.h:25-87`, `Private/SOTS_FXManagerSubsystem.cpp:1-180`).
- **Public API surface**: tag triggers (`TriggerFXByTagWithReport`/`TriggerFXByTag`/`TriggerAttachedFXByTag`), legacy `PlayCueByTag`/`PlayCueDefinition`, stubbed `RequestFXCue` (`SOTS_FXManagerSubsystem.h:87,123-148`; cpp `246-396`).
- **Definition sources**: in-memory `CueMap` (only via `RegisterCue`) and data-asset libraries (`Libraries` array -> `RegisteredLibraryDefinitions`) (`SOTS_FXManagerSubsystem.h:93-151`, cpp `59-95,271-291`).
- **Routing result surface**: `ProcessFXRequest` resolves tag -> definition -> broadcasts `OnFXTriggered` with `FSOTS_FXResolvedRequest`; no spawning in this path (`SOTS_FXManagerSubsystem.cpp:396-468`).
- **Pooling path**: old cue definitions spawn Niagara/Audio/Camera directly via `SpawnCue_Internal`; pools are per-cue arrays with Acquire helpers (`SOTS_FXManagerSubsystem.cpp:493-552,530-572,622-690`).
- **Data definitions**: `FSOTS_FXDefinition` soft references + defaults; `USOTS_FXDefinitionLibrary` list; `FSOTS_FXRequest`/`Result` payloads and `OnFXTriggered` delegate (`Public/SOTSFXTypes.h:15-155`, `Public/SOTS_FXDefinitionLibrary.h:5-21`).
- **BP surface**: thin wrappers in `USOTS_FXBlueprintLibrary` (get manager, PlayCue*, TriggerFXWithReport, StopFXHandle, debug getters) (`Public/SOTS_FXBlueprintLibrary.h:9-46`, `Private/SOTS_FXBlueprintLibrary.cpp:1-74`).
- **BEP exports**: FX currently requested by direct asset wiring (Niagara/Sound/CameraShake) in BPs such as `BEP_EXPORTS/SOTS/2025-12-12_09-24-57/Blueprints/BP_SOTS_VFX_SPAWNER/Flow.txt` and `.../CBP_SandboxCharacter_UE4_Mannequin/Snippets/EventGraph.snippet.txt` (manual `Set Niagara System Asset`, `PlaySoundAtLocation`, `CameraShake`), not via tag router.

## Tag resolution & routing (as-is)
- `BuildRegistryFromLibraries` loads `Libraries` once on init; first definition wins, optional dev warnings, no priority/async (`SOTS_FXManagerSubsystem.cpp:59-95`).
- `FindDefinition` only consults `RegisteredLibraryDefinitions` and warns (dev-only) if missing; `RequestFXCue`/`Trigger*` all flow into `ProcessFXRequest` which stops on invalid tag/registry-not-ready/definition-not-found/missing-attachment (`SOTS_FXManagerSubsystem.cpp:271-437`).
- Legacy cue spawning (`PlayCueByTag`/`PlayCueDefinition`) uses `CueMap` registered at runtime; not auto-populated from libraries (`SOTS_FXManagerSubsystem.cpp:135-205`).
- `BroadcastResolvedFX` only broadcasts resolved payload; spawning is left to external listeners (`SOTS_FXManagerSubsystem.cpp:293-345`).

## Pooling / lifecycle (current state)
- Pools store arrays per cue; `AcquireNiagaraComponent`/`AcquireAudioComponent` reuse inactive/non-playing components else spawn new with `bAutoDestroy=false`; no max size, no expiry, no finished callbacks, and `Deinitialize` just resets maps without stopping components (`SOTS_FXManagerSubsystem.cpp:493-575,630-689`).
- `FSOTS_FXCueDefinition::bLooping/bAutoDestroy` are not honored in pooled path; Niagara/Audio are always kept alive for reuse (`SOTS_FXManagerSubsystem.cpp:610-690`).

## Diagnostics / gating (current)
- Dev flags default mostly false; only failure logging default true (`bLogFXRequestFailures`) and guarded by shipping/test macros (`SOTS_FXManagerSubsystem.h:100-118`, cpp `476-490`).
- Global toggles (Blood/HighIntensity/CameraMotion) exist and round-trip to profile but are not consulted during request processing (`SOTS_FXManagerSubsystem.h:132-148`, cpp `200-241,396-468`).

## Missing behaviors (with proof)
- **Stubbed Request path**: `RequestFXCue` is void, builds minimal `FSOTS_FXRequest`, no handle/result, no caller feedback (`SOTS_FXManagerSubsystem.cpp:246-264`).
- **No deterministic library priority**: libraries searched in array order; duplicates silently skipped unless dev flag; no per-library priority or versioning (`SOTS_FXManagerSubsystem.cpp:59-95`).
- **No spawn in tag router**: `ProcessFXRequest` only broadcasts resolved payload; relies on external listeners; no built-in Niagara/Audio/camera spawn or handle return (`SOTS_FXManagerSubsystem.cpp:396-468`).
- **Pooling gaps**: pooled path has no cap/cleanup and ignores cue `bAutoDestroy`/`bLooping`; components not stopped on deinit (Acquire/SpawnCue_Internal blocks at `493-575,630-690`).
- **Toggles unused**: blood/high-intensity/camera flags never gate definitions or requests (flags defined `SOTS_FXManagerSubsystem.h:132-148`; absence of usage throughout `ProcessFXRequest`/Spawn path).
- **BEP caller expectations unclear**: exports show manual Niagara/Sound/CameraShake usage, no tag-based requests; need to align contract for async vs sync vs handles (`BEP_EXPORTS/SOTS/.../BP_SOTS_VFX_SPAWNER/Flow.txt`, `.../CBP_SandboxCharacter_UE4_Mannequin/Snippets/EventGraph.snippet.txt`).

## SPINE plan (next passes)
- **SPINE_1: RequestFXCue + result contract** — convert `RequestFXCue` into handle/report-capable entry; unify Trigger/Request path with explicit status/diagnostics (defaults off) and non-silent failures.
- **SPINE_2: Registry/library resolution** — deterministic priority order, duplicate handling, optional async soft-load; clear fallback rules and failure telemetry.
- **SPINE_3: Spawn execution polish** — integrate tag router with actual Niagara/Audio/Camera spawn (with soft ref handling) or intentionally document broadcast-only; attach/orientation/scale rules and context safety.
- **SPINE_4: Pooling + lifecycle** — pool caps, return-to-pool/stop semantics, honor bAutoDestroy/bLooping, cleanup on deinit; optional metrics.
- **SPINE_5: Toggles + overrides** — apply blood/intensity/camera gates and per-cue overrides; BEP-aligned behavior.
- **SPINE_6: Shipping/Test sweep + docs** — guard logs, remove dead TODOs, finalize BP helpers and worklogs, delete Binaries/Intermediate after each spine.
