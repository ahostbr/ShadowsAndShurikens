# SOTS KEM Completeness Pass — SPINE2 (2025-12-14 09:50)

## Summary
- Added optional auto position-tag classifier that injects SOTS.KEM.Position.* into evaluation ContextTags when enabled (defaults OFF).
- Corner classification and thresholds are tunable; vertical uses a threshold and exits early to avoid mixing with ground tags.
- Kept caller tags intact; classifier only runs when no position tags already exist.

## Config (defaults)
- `bAutoComputePositionTags = false`
- `bAutoComputeCornerTags = true`
- `AutoPositionVerticalThreshold = 30.f`
- `AutoPositionCornerDotThreshold = 0.35f`

## Evidence pointers
- Helper definition: `SOTS_KEM_ManagerSubsystem.cpp` -> `ComputeAndInjectPositionTags` (adds tags if none present, vertical/ground logic with debug logging).
- Call sites (effective ContextTags): `SOTS_KEM_ManagerSubsystem.cpp` in request loop and `RunKEMDebug` (ExecContext.ContextTags passed through classifier when flag enabled).

## Behavior impact
- OFF by default; no gameplay change unless `bAutoComputePositionTags` is enabled.
- Caller-supplied position tags are respected; classifier returns immediately if any position tags are present.

## Usage
- Enable `bAutoComputePositionTags` in KEM config to auto inject SOTS.KEM.Position.* tags based on instigator/target relative pose. Adjust corner/vertical thresholds as needed.
