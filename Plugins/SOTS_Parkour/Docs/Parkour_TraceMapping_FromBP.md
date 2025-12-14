# Parkour Trace Mapping: CGF Blueprint → SOTS_Parkour C++

Reference Blueprint: `BlueprintExports/ParkourComponent_Readable.txt`
Target C++: `Plugins/SOTS_Parkour/Source/SOTS_Parkour/` (primarily `SOTS_ParkourComponent.cpp`, `SOTS_ParkourSurfaceProbes.*`, `SOTS_ParkourVerticalTraceLibrary.*`)

| Blueprint function (ParkourComponent) | C++ equivalent | Description | Notes |
| --- | --- | --- | --- |
| Get First Capsule Trace Settings | `BuildFirstCapsuleTrace_NotBusy` / `BuildFirstCapsuleTrace_Climb` (in `SOTS_ParkourComponent.cpp`) | Computes the primary forward capsule sweep used for initial detection; NotBusy branch speed-lerps distance, Climb branch uses climb arrow offsets. | Uses `FSOTS_ParkourTraceProfile`; capsule radius/half-height pulled from profile; forward distance lerped by speed; Z offset ~+75 (NotBusy). |
| Get Second Capsule Trace Settings | `BuildSecondCapsuleTrace` (in `SOTS_ParkourComponent.cpp`) | Builds the follow-up forward capsule sweep for vault confirmation after the first hit. | Now data-driven via `Parkour.TraceProfile.ConfirmForward`: default forward offset -40 (into wall along -ImpactNormal), vertical offset -80, radius 25, half-height 82. Falls back to radius = max(20, half of first-trace span) only if profile radius is unset. |
| Check Mantle Surface | `ConfirmSurfaceForAction` (Mantle path) | Validates mantle surface with capsule sweep offsets (mantle ±8 Z); writes World/Target/Surface normals. | Uses `USOTS_ParkourSurfaceProbes::ProbeMantleConfirm` via `ConfirmSurfaceForAction`; OmniTrace capsule sweep; debug draw optional. |
| Check Vault Surface | `ConfirmSurfaceForAction` (Vault path) | Validates vault surface with forward/up offsets (+18/+5 Z); similar to mantle confirm. | Uses OmniTrace capsule sweep; applies vault nudge before confirm. |
| Check Climb Surface | `RunWallGridProbes` | Performs horizontal/vertical grid to refine wall depth, height delta, and XY distance for climb-classification. | Grid counts/ranges pulled from config (`VerticalWallTraceQuantity_*`, `HorizontalWallTraceRange`, etc.). |
| Check Predict Jump | `TryDetectLedgeMoveAndHops` (predictive branch) | Predictive forward hop grid when speed is high; chooses PredictJump action on valid forward probe. | Uses sphere sweeps (r=5) with extended forward distances 180–200uu and down-depth scaled by capsule/grid. |
| Check Back Hop | `TryDetectLedgeMoveAndHops` (back-hop branch) | Sweeps backward off the wall with vertical lift to validate a back hop; sets BackHop action. | Sphere sweep radius 5; backward distance 140uu; vertical lift 25uu; DirectionTag set to Backward. |
| Check Ledge Move | `TryDetectLedgeMoveAndHops` (sideways branch) | Side probes along the wall to shift left/right while braced; sets LedgeMove action. | Sphere sweep radius 5 at Z+25; distance = `LedgeMoveHorizontalDistance`; DirectionTag left/right via `MakeDirectionTag`. |
| Check Corner Move | `TryDetectCornerMove` | Side-step + forward probe around corners, then down-probe to new ledge; sets CornerMove. | Side offset = `HorizontalWallTraceRange*2`; forward = `LedgeMoveHorizontalDistance`; back offset -40; rejects normals with dot > 0.6; down depth tied to vertical grid; DirectionTag from side offset. |
| Check TicTac | `TryTicTacSideProbes` | Side sweeps from wall normal to find a TicTac surface when mantle/drop fail. | Sphere sweep radius 30; distance = capsule radius * 2.5; sets ResultType Mantle, ClimbStyle Braced. |
| Check Mantle/Vault Classification | `TryDetectSimpleParkourOpportunity` + `ConfirmSurfaceForAction` + `ClassifyAdvancedParkour` | Overall mantle/vault selection after first trace and height gates. | Uses `USOTS_ParkourVerticalTraceLibrary` for vertical start height; applies MaxSafeDrop / mantle thresholds from config. |
| Find Drop Down Hang Location | `RunDropHangProbes` | Primary (r=35) + secondary (r=5) downward sphere traces from first hit to find hang point. | Accepts either hit; writes TargetLocation/SurfaceNormal from chosen hit. |
| Check Drop/Hang | `TryDetectParkourOnce` flow → `RunDropHangProbes` | Drop path when height delta negative but within MaxSafeDrop; sets Drop action. | ClimbStyle set to FreeHang; ResultType Drop. |
| Check Corner Hop / OutCorner Movement | `TryDetectLedgeMoveAndHops` (predictive grid) | Corner/predictive hop uses same grid as PredictJump (forward offsets per HorizontalWallTraceRange). | DirectionTag from RightOffset; down-probe depth uses vertical grid counts. |
| Add/Finish/Camera Timeline (camera helpers) | Not yet ported (no direct C++ equivalent) | Camera timeline remains BP-only; no C++ implementation in SOTS_Parkour. | Marked TODO for future parity. |
| Beam Hidden checks (Parkour.State.BeamHidden) | No dedicated C++ path yet | State tag exists in enums; no detection/execution in `SOTS_ParkourComponent`. | Listed as partial parity in docs. |

## Notes
- All C++ sweeps route through OmniTrace helpers (`RunCapsuleTraceWithOmniTrace`, `RunSphereTraceWithOmniTrace`) or `USOTS_ParkourSurfaceProbes`.
- Direction tags are set during detection (`MakeDirectionTag` or branch-specific assignments) and preserved into `FinalizeResultTags`.
- Camera timeline and BeamHidden behaviors are absent in C++; any future port should mirror the BP graphs noted above.
