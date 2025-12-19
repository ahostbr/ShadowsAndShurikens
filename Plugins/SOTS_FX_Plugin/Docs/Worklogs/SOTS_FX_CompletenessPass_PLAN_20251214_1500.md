# SOTS_FX_Plugin — Planned-Behaviors Completeness Sweep (PLAN)

## Current state (evidence)
- **Subsystem entry**: `USOTS_FXManagerSubsystem` (`Public/SOTS_FXManagerSubsystem.h`, `Private/SOTS_FXManagerSubsystem.cpp`).
- **Manual cue registration only**: `RegisterCue` writes to `CueMap`; no validation/auto-discovery (`SOTS_FXManagerSubsystem.cpp`).
- **Definition lookup**: `FindDefinition` iterates `Libraries` array; no duplicate/missing validation, no failure logging (`SOTS_FXManagerSubsystem.cpp`).
- **Trigger routing**: `TriggerFXByTag`/`TriggerAttachedFXByTag` resolve then broadcast `OnFXTriggered` only; no spawn/asset load/resolution result; `RequestFXCue` just forwards (`SOTS_FXManagerSubsystem.cpp`).
- **Pooled spawn path**: `PlayCueByTag`/`PlayCueDefinition` -> `SpawnCue_Internal`; pools Niagara/Audio per cue, no caps or lifecycle tracking, no finished delegate (`SOTS_FXManagerSubsystem.cpp`, pooling helpers `AcquireNiagaraComponent`, `AcquireAudioComponent`, `ApplyContext...`).
- **Active tracking**: only counts via `GetActiveFXCountsInternal`; no active handle map or cleanup on deinit beyond `CuePools.Reset()` (no Stop/Destroy) (`SOTS_FXManagerSubsystem.cpp`).
- **Config/toggles**: simple bools for Blood/HighIntensity/CameraMotion with profile round-trip; not applied to routing (`SOTS_FXManagerSubsystem.cpp`).
- **Blueprint surface**: `USOTS_FXBlueprintLibrary` is thin wrappers; no result codes or diagnostics (`SOTS_FXBlueprintLibrary.cpp`).
- **Definitions**: `FSOTS_FXDefinition` + `USOTS_FXDefinitionLibrary` are data-only tag->asset lists; no validation (`Public/SOTSFXTypes.h`, `Public/SOTS_FXDefinitionLibrary.h`).
- **Debug/diagnostics**: none; no dev-only logging or missing-tag warnings.

## Missing behaviors / gaps (with evidence pointers)
- **Registry/discovery**: No validation of `Libraries` or `CueMap` on init; no duplicate/missing tag detection; manual only (FindDefinition loop, RegisterCue, `SOTS_FXManagerSubsystem.cpp`).
- **Routing guarantees / result contract**: Trigger path only broadcasts; no explicit resolve failures, no toggle gating, no unified return struct (`TriggerFXByTag`, `BroadcastResolvedFX`).
- **Pooling policy**: No pool size caps, reuse only on inactive; audio/niagara spawned without max limits; no cleanup on Deinitialize (pool reset without stopping) (`AcquireNiagaraComponent`, `AcquireAudioComponent`).
- **Lifecycle/finished events**: No active handle tracking or finished callbacks; handles only carry component pointers; no cleanup on world teardown beyond map reset (`SpawnCue_Internal`, `Deinitialize`).
- **Diagnostics**: No dev-only warnings for missing tags/assets; no shipping/test guards needed yet because no logs.

## Spine plan (next passes)
1) **SPINE_1: Registry/discovery + validation**
   - Add init-time validation of `Libraries` and manual `CueMap` (duplicate/missing/invalid assets).
   - Optional dev-only warning toggle for missing tags; no runtime behavior change.
2) **SPINE_2: Unified routing + explicit result contract**
   - Introduce Resolve/Trigger result enum + struct; ensure all Trigger/Request paths flow through one resolver applying toggles; explicit failure reasons + dev-log guard.
3) **SPINE_3: Pooling/lifecycle policy**
   - Add pool caps (per cue) and cleanup; stop/destroy pooled components on deinit; track active handles (tag, spawned time, weak refs).
4) **SPINE_4: Finished events + cleanup**
   - Track active FX handles, bind finish events (audio/niagara), BP delegate `OnSOTSFXCueFinished`; cleanup on finish/world teardown.
5) **SPINE_5: Dev-only diagnostics**
   - Add guarded debug logging/dumps; optional missing-tag warnings; ensure shipping/test guards.

## Default behavior promise
- All additions will default to current behavior (no routing/pooling changes unless toggled).
- Diagnostics/dev logs will be gated and off by default; no shipping impact.

## Next steps
- Execute SPINE_1 in next pass (registry validation + missing-tag warning toggle) while keeping runtime unchanged by default.
