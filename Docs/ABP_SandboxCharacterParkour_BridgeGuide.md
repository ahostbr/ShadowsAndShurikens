# ABP_SandboxCharacterParkour → Parkour Data Bridge Guide (Updated)

Goal: keep the legacy AnimBP on its native parent (no `USOTS_ParkourAnimInstance` exists now), bind parkour/IK variables 1:1, and trim redundant Blueprint logic without touching `.uasset` contents beyond normal Blueprint editing.

Quick checklist:
- [ ] Verify AnimBP parent is your intended base (not `USOTS_ParkourAnimInstance`, which was removed).

Inherited C++ variables mirror the legacy ABP names. Bind graph references directly to these:
- **Parkour / IK**: `ParkourIK_Enabled`, `LegIK`, `LeftHandLedgeLocation`, `RightHandLedgeLocation`, `LeftHandLedgeRotation`, `RightHandLedgeRotation`, `LeftFootLocation`, `RightFootLocation`, `bLeftClimbIK`, `bRightClimbIK`, `IsHidingOnBeam`, `ClimbMovement`, `ParkourState`, `ParkourLogicalState`, `ParkourAction`, `ClimbStyle`, `ClimbDirection`.
- **Movement / cover**: `CurrentMovementX`, `CurrentMovementY`, `PlayersForwardVector`, `RightArmBlendWeight`, `CoverState`, `CoverMovingDirection`, `IsPlayerCrouched`, `IsInCover_ABP`.
- **Essential locomotion**: `Velocity`, `Speed2D`, `HasVelocity`, `LastNonZeroVelocity`, `Velocity_LastFrame`, `Acceleration`, `AccelerationAmount`, `HasAcceleration`, `VelocityAcceleration`, `CharacterTransform`, `RootTransform`, `OffsetRootBoneEnabled`.
- **State tags / enums**: `MovementMode`, `MovementMode_LastFrame`, `MovementState`, `MovementState_LastFrame`, `Gait`, `Gait_LastFrame`, `Stance`, `Stance_LastFrame`, `RotationMode`, `RotationMode_LastFrame`.
- **Motion Matching scaffold**: `MMDatabaseLOD`, `CurrentSelectedDatabase`, `CurrentDatabaseTags`, `HeavyLandSpeedThreshold`, `Trajectory`, `TrajectoryCollision`, `FutureVelocity`, `PreviousDesiredControllerYaw`.

## AnimGraph wiring
1) Keep the existing AnimGraph flow: `OutputPose` → `Default Slot` → `IK_Mixamo_Fix` → `ParkourIK` → `LegIK`.
2) `ParkourIK` node: enable pin should use `ParkourIK_Enabled`; positions/rotations use the inherited hand/foot targets.
3) `LegIK` node: enable pin should use `LegIK`. If the node exposes "Offset Root Bone", use `OffsetRootBoneEnabled`.

## Event Graph notes
- If the Event Graph has heavy value-setting logic (velocity/accel/trajectory), you can remove those setters after pins are rebound; C++ now updates them each tick.
- Preserve the call order: `Update Essential Values` → `Generate Trajectory` → `Update States` (mirrors the legacy readable export).
- Any Motion Matching nodes that expect a PoseSearch `Trajectory` struct can read `Trajectory` (array) and `FutureVelocity`/`TrajectoryCollision` placeholders for now. Replace with real MM data once the GASP/MM bridge is ready.

## Common compile issues (and fixes)
- **Missing variable after reparent**: delete the old BP variable with the same name; the inherited one will appear. Reconnect pins.
- **Wrong type pin**: if a pin expects a PoseSearch struct, temporarily use `Trajectory` (array) + conversion nodes, or keep the BP-specific trajectory builder until MM C++ lands.
- **Interface calls**: ensure `Target` pins on interface calls point to `self` (the AnimBP), not a null reference.
- **Enum mismatches**: if Blueprint enums changed, refresh node and reselect values.

## Minimal regression checklist
- AnimGraph compiles with no errors/warnings.
- Preview: toggling `ParkourIK_Enabled` and `LegIK` shows expected IK nodes firing.
- Variables read correctly in the Debug Details panel while PIE (Velocity, Speed2D, HasAcceleration, Trajectory length > 0 when moving).

Document owner: Animation bridge workstream. Update this doc if variable names or bindings change.
