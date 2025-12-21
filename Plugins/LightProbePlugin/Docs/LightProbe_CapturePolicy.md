# Light Probe Capture Policy

## Capture-on-demand rationale
- The probe now only captures when we explicitly need a sample rather than continuously every frame or when the capture component moves. This keeps GPU work low and avoids redundant SceneCapture traffic when the probe simply sits above the player.
- `UpdateLightLevel` drives capture: it calls `CaptureScene` once per timer tick right before sampling the render target, so the probe still publishes the same calibrated light level but only on the configured interval.

## Flags & properties changed
- `ProbeCapture->bCaptureEveryFrame` and `ProbeCapture->bCaptureOnMovement` are both `false`, ensuring no implicit capture requests outside the timer.
- The timer now only starts when both the capture component and the render target exist, avoiding ticks when a capture cannot run.
- The probe mesh is now `CastShadow=false`, `bCastHiddenShadow=false`, and hidden-in-game by default; `sots.lightprobe.ShowProbeMesh` toggles its visibility for debugging while keeping shadows disabled.
- A manual `CaptureScene` call happens before each readback so the render target always reflects the most recent frame at the sampling moment.

## Debug CVars
- `lightprobe.DebugWidget`: existing dev-only widget that renders the RT preview on screen (still gated by `AreDebugWidgetsAllowed`).
- `sots.lightprobe.ShowProbeMesh`: newly introduced CVar (default `0`) to un-hide the probe mesh when a visual reference is necessary. Shadows remain disabled even when the mesh becomes visible.

## Downstream expectations
- Nothing changes for `USOTS_PlayerStealthComponent` or the GSM pipeline: `UpdateLightLevel` still reads the same pixels (the same render target resolution/format) and feeds normalized light levels into `PlayerStealthComponent::SetLightLevel` at the configured interval.