# Buddy Worklog — FX overflow logging and contract documentation

## Goal
- Ensure SOTS_FX_Plugin uses a dedicated log category, makes pool overflow rejections explicit via logs, and document the one-shot request contract for future authors.

## What changed
- Added the `LogSOTS_FX` declaration/definition so every `[SOTS_FX]` message passes through a single channel and updated every `UE_LOG(LogTemp, …)` statement accordingly.
- Created `LogPoolOverflow` and invoked it in `AcquireNiagaraComponent`/`AcquireAudioComponent` whenever the per-cue or total pool caps cause a rejection; log output is now tied to the plugin category and gated by `LogSOTS_FX`.
- Authored `SOTS_FX_OneShotContract.md` to capture the `TriggerFXByTag*` → `ProcessFXRequest` → `ExecuteCue` flow, the `FSOTS_FXRequestReport` failure contract, and the new overflow logging expectations.

## Files changed
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Docs/SOTS_FX_OneShotContract.md

## Notes + risks/unknowns
- Changes are purely C++/doc updates and have not been built; overflow logging runs only in non-shipping/test builds so production pipelines will not see the new warnings unless `bLogPoolActions` is enabled.
- The broader repo already has unrelated changes under `DevTools/python/…`; those were untouched.

## Verification status
- Not run (per instructions on avoiding builds).

## Follow-ups / next steps
1. Run a non-shipping run to verify the new `LogSOTS_FX` messages appear when pools hit `MaxActivePerCue` or `MaxPooled*`.  
2. Confirm the new one-shot contract doc addresses any remaining questions about `FSOTS_FXRequestReport` usage with Blueprint callers.  
3. Clean up plugin binaries/intermediate directories once edits are complete.
