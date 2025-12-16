# SOTS_FX_Plugin SPINE5 — Policy & Profile

**Goal**
Centralize FX policy decisions, honor profile toggles, and expose per-cue overrides without adding new spawn features.

**Toggles & Sources**
- Profile slice: `FSOTS_FXProfileData` (blood, high-intensity, camera-motion) from `SOTS_ProfileShared`.
- Subsystem caches toggles and exposes `ApplyFXProfileSettings()` for profile updates.

**Policy Rules (EvaluateFXPolicy)**
- ForceDisable override: block entirely (`DisabledByPolicy`).
- RespectGlobalToggles (default):
  - `bBloodEnabled=false` + cue `bIsBloodFX=true` → block.
  - `bHighIntensityFX=false` + cue `bIsHighIntensityFX=true` → block.
  - `bCameraMotionFXEnabled=false` + cue shake (unless `bCameraShakeIgnoresGlobalToggle=true`) → shake skipped; other outputs continue.
- IgnoreGlobalToggles: bypass all global gating.
- Debug message includes `ShakeSkipped` when only the shake is suppressed.

**Per-cue Overrides**
- Fields on FX definitions and legacy cue definitions:
  - `ESOTS_FXToggleBehavior ToggleBehavior` (Respect/Ignore/ForceDisable)
  - `bIsBloodFX`, `bIsHighIntensityFX`, `bCameraShakeIgnoresGlobalToggle`
- Defaults preserve prior behavior (Respect toggles, not blood/high-intensity, shake respects camera toggle).

**Surface / Reporting**
- Tag-driven requests run `EvaluateFXPolicy()` before spawn; failures return `DisabledByPolicy` with reason in `DebugMessage`.
- Shake-only cues blocked by camera toggle return `DisabledByPolicy` and log `ShakeSkipped` (dev builds).
- Successful requests that skip shake also note `ShakeSkipped` in `DebugMessage` (dev builds only).

**Evidence Pointers**
- Toggle behavior enum and per-cue policy fields: `SOTSFXTypes.h` ([lines 66-78](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h#L66-L78), [lines 189-210](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h#L189-L210)); legacy cue fields in `SOTS_FXCueDefinition.h` ([lines 13-53](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXCueDefinition.h#L13-L53)).
- Profile apply entry point: `ApplyFXProfileSettings` declaration/impl ([SOTS_FXManagerSubsystem.h#L186-L207](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h#L186-L207), [SOTS_FXManagerSubsystem.cpp#L508-L528](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L508-L528)).
- Central policy evaluation usage + implementation: call in request path ([SOTS_FXManagerSubsystem.cpp#L742-L776](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L742-L776)), function body ([SOTS_FXManagerSubsystem.cpp#L823-L879](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L823-L879)).
- ExecuteCue gating & shake skip handling: [SOTS_FXManagerSubsystem.cpp#L1089-L1132](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1089-L1132).
- Legacy pooled path honoring overrides: [SOTS_FXManagerSubsystem.cpp#L1693-L1742](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1693-L1742).

**Notes**
- No builds/tests run (per instruction).
- Camera shake suppression is non-fatal; FX still spawns if allowed. Force-disable or toggle blocks return `DisabledByPolicy` with reason.
