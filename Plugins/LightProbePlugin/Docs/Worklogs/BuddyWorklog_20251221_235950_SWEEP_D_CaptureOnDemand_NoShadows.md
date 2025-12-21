# Buddy Worklog — Capture-On-Demand, No Shadows (20251221_235950)

Goal
- Stop the LightProbe from SceneCapturing every frame while keeping the timer-driven sampling that powers GSM-aware stealth.

What changed
- Previously the probe SceneCapture component ran with `bCaptureEveryFrame=true` / `bCaptureOnMovement=true`, so the GPU kept updating the RT even if the timer had not asked for a new sample.
- Now we only tick the timer when the capture component and render target exist, we explicitly call `CaptureScene()` from `UpdateLightLevel`, and the capture component is configured to stay idle between ticks.
- The probe mesh no longer casts shadows, is hidden in-game by default, and can be revealed via `sots.lightprobe.ShowProbeMesh` when a visual reference is required; the mesh still can’t produce playable shadows.

Files changed
- [Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp](Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp)
- [Plugins/LightProbePlugin/Docs/LightProbe_CapturePolicy.md](Plugins/LightProbePlugin/Docs/LightProbe_CapturePolicy.md)

Notes + risks/unknowns
- Render target defaults: 512×512, format `RTF_RGBA16f` (same as before). Timer sampling still feeds the normalized value into `USOTS_PlayerStealthComponent::SetLightLevel` after `CaptureScene` + readback, so downstream code sees identical data.
- Semantics preserved: upstream callers still get a light-level update every `ProbeUpdateInterval`, but the GPU now only works when we explicitly request a sample.

Verification status
- UNVERIFIED (no build/run).

Cleanup
- Plugins/LightProbePlugin/Binaries/ deleted.
- Plugins/LightProbePlugin/Intermediate/ deleted.

Follow-ups / next steps
- Ryan: run a gameplay session to ensure the timer-driven capture still produces consistent `PlayerStealthComponent` updates and to confirm `sots.lightprobe.ShowProbeMesh` reveals a shadowless proxy when needed.