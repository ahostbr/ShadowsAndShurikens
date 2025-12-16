# SOTS_FX_Plugin SPINE3 Execution Pipeline (2025-12-16 14:35 UTC)

## Goal
- Unify tag-driven FX spawning with consistent transform/attachment rules, Niagara/audio/shake coverage, and reliable handle reporting (no pooling changes).

## Notes / Changes
- Execution params now resolve attachment intent (socket + fallback to target/instigator roots) and seed world transforms when location/rotation are absent before spawning.
- ExecuteCue applies surface alignment, converts to parent-relative space for attachments, applies offsets, and reconverts to world for reporting while spawning Niagara/audio with the relative transform. Spawned object handle prefers Niagara, otherwise audio.
- Niagara parameter injection (float/vector/color) remains centralized post-spawn; audio spawns honor attenuation, concurrency, volume, and pitch; camera shakes respect global gating with per-cue override.

## Evidence
- Public entry points funnel through ExecuteCue via ProcessFXRequest: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L807-L853](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L807-L853)
- Execution params + attachment resolution: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L977-L1023](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L977-L1023)
- Transform pipeline + relative attachment spawning and handle/reporting: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1207-L1306](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1207-L1306)
- Niagara parameter injection post-spawn: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1120-L1143](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1120-L1143)
- Audio spawn with attenuation/concurrency and pitch/volume: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1161-L1183](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1161-L1183)
- Camera shake gating and trigger: [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L890-L925](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L890-L925) and [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1187-L1199](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1187-L1199)

## Verification
- Not run (per instructions).

## Cleanup
- Plugin binaries/intermediate will be removed post-edit.
