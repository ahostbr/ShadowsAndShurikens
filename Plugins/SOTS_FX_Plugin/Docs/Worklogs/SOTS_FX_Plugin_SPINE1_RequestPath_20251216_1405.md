# Buddy Worklog — SOTS_FX_Plugin SPINE1 Request Path

**Goal**: Remove RequestFXCue dead-ends and ensure cue triggers return explicit reports without breaking other FX entrypoints.

**Discovery**
- BEP search (`DevTools/python/ad_hoc_regex_search.py`, pattern RequestFXCue/TriggerFX/FXCue/TriggerAttachedFX, root BEP_EXPORTS) found only CGF audio cues; no BEP callsites for RequestFXCue.
- Public FX entrypoints live in `USOTS_FXManagerSubsystem` and `USOTS_FXBlueprintLibrary`; resolution/policy already centralised via `TryResolveCue` and `EvaluateFXPolicy`.

**Changes**
- `RequestFXCue` now returns `FSOTS_FXRequestReport` (no silent void path) and delegates to the shared request pipeline.
- Added a blueprint helper `RequestFXCue` to retrieve the full report when invoking cues from BP code.
- `RequestFXCueWithReport` now forwards through the primary path to avoid duplication.

**Files touched**
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXBlueprintLibrary.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXBlueprintLibrary.cpp

**Verification**
- Not run (per instructions). Request path now always returns an `FSOTS_FXRequestReport` to callers, and BP helper covers blueprint usage.

**Cleanup**
- Deleted Plugins/SOTS_FX_Plugin/Binaries and Plugins/SOTS_FX_Plugin/Intermediate.

**Follow-ups**
- None noted; resolver/policy already centralised in `TryResolveCue`/`EvaluateFXPolicy`.
