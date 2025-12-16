# SOTS_FX_Plugin — SPINE3 Execution Pipeline

**Goal**
- Unify tag-driven FX execution behind a single `ExecuteCue` path with consistent transform/attach handling, Niagara params, audio settings, and camera shake gating. Provide spawned-object handles via reports.

**What changed**
- Added per-request surface normal support, per-definition offsets/alignment, audio attenuation/concurrency tuning, Niagara user parameter maps, and camera shake metadata on FX definitions.
- Introduced `FSOTS_FXExecutionParams` and a unified `ExecuteCue` pipeline that all tag-based entrypoints call, handling resolve → policy → transform → spawn → report.
- Spawn logic now handles world vs attached spawns, optional surface alignment, offset application, Niagara parameter injection, audio attenuation/concurrency, and camera shake gated by global toggle.
- Reports return the spawned object (Niagara preferred, else Audio) and legacy results are populated from the unified execution path.

**Key references**
- Execution params + pipeline + attach/transform flow: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L604-L746](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L604-L746)
- Niagara/audio spawn + parameter injection + camera shake gating: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L665-L742](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L665-L742)
- New definition fields (offsets, alignment, params, audio/shake metadata): [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h#L67-L131](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h#L67-L131)
- Request supports surface normals for alignment: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h#L40-L56](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h#L40-L56)
- Entry points route through `ProcessFXRequest` → `ExecuteCue`: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L594-L652](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L594-L652)

**BEP scan notes**
- SOTS BPs use `PlaySoundAtLocation`, `PlaySound2D`, `K2_AttachToComponent`, and CameraShake usage; no special Niagara parameter patterns observed.

**Verification**
- No builds/cooks/tests run (per instructions).

**Cleanup**
- Issued deletion command for `Plugins/SOTS_FX_Plugin/Binaries` and `Intermediate` (use Test-Path guarded removal to ensure success if rerun).
