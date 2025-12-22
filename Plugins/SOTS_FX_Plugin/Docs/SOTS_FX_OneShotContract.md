# SOTS_FX One-Shot API Contract

## 1. Entry points
- [TriggerFXByTagWithReport](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L711) is the canonical call that returns an `FSOTS_FXRequestReport` so callers can inspect `ESOTS_FXRequestResult`, `DebugMessage`, and the resolved tag.
- [TriggerFXByTag](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L721) is the blueprint-friendly wrapper that forwards its parameters into `ProcessFXRequest` and ignores the legacy handle step.

## 2. Execution pipeline
- [ProcessFXRequest](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L770) validates the world/registry, resolves the cue tag, builds `FSOTS_FXExecutionParams`, checks policy, and only then calls `ExecuteCue`.
- [ExecuteCue](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1176) applies surface alignment/offsets, configures or attaches spawned components, triggers camera shake, and returns the final `FSOTS_FXRequestReport` with the success/failure state.

## 3. Failure contract
- Every path populates `FSOTS_FXRequestReport.Result` and optional debug text so upstream callers always know "what" failed and "why".
- [MaybeLogFXFailure](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L880) emits a warning via `LogSOTS_FX` when `bLogFXRequestFailures` is enabled in dev builds so the failure context can be surfaced without changing the public API.

## 4. Pooling & overflow behavior
- [AcquireNiagaraComponent](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1571) and [AcquireAudioComponent](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1690) guard against exceeding `MaxActivePerCue` and enforce the per-pool `MaxPooledNiagaraComponents`/`MaxPooledAudioComponents` caps before handing off a component to `ExecuteCue`.
- Rejections set `bOutRejectedByPolicy`, skip spawning, and trigger [LogPoolOverflow](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1818) (active only in non-shipping builds) so engineers can correlate the warning with the tag, component type, and cap that blocked the request.

## 5. Logging contract
- All `[SOTS_FX]` logs now use the single, shared category declared at [LogSOTS_FX declaration](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h#L13) and defined near the top of the CPP file [here](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L23).
- `LogPoolEvent`, `MaybeLogFXFailure`, and the new `LogPoolOverflow` helper consolidate the debugging surface under that category so the FU log filters stay consistent across build configs.
