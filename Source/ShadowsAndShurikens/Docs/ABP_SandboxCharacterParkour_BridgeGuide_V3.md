# ABP Sandbox Character Parkour — Bridge Guide V3 (Updated)

`USOTS_ParkourAnimInstance` has been removed. Keep your AnimBP parented to its native base (e.g., the CGF/GASP AnimInstance) and only **read** parkour/IK data exposed by `USOTS_ParkourComponent`/`FSOTS_ParkourResult`. Do not write back into C++ properties.

## One-time editor steps
1. Open `ABP_SandboxCharacter` (or `ABP_SandboxCharacterParkour` if that asset is in use).
2. In Class Settings, choose your intended native parent (no SOTS C++ AnimInstance exists now).
3. Right-click the graph → **Refresh All Nodes**.
4. Compile once to surface any stale nodes before cleanup.

## Delete all Set nodes that target C++ fields
For each function/sequence (`Update Animation`, `SetReferences`, `UpdateEssentialValues`, `UpdateStates`, `GenerateTrajectory`, `Update_MotionMatching`, parkour handlers, etc.), delete `Set` nodes that write to the C++ properties below. Remove any now-dead calculations feeding them. The AnimGraph should keep reading these values, but the writes must disappear.

- Locomotion / cover: `CurrentMovementX`, `CurrentMovementY`, `PlayersForwardVector`, `RightArmBlendWeight`, `CoverState`, `CoverMovingDirection`, `IsPlayerCrouched`, `IsInCover_ABP`.
- Essential values: `Velocity`, `Velocity_LastFrame`, `LastNonZeroVelocity`, `Speed2D`, `HasVelocity`, `Acceleration`, `AccelerationAmount`, `HasAcceleration`, `VelocityAcceleration`, `CharacterTransform`, `RootTransform`, `OffsetRootBoneEnabled`.
- Essential states: `MovementMode`, `MovementMode_LastFrame`, `MovementState`, `MovementState_LastFrame`, `Gait`, `Gait_LastFrame`, `Stance`, `Stance_LastFrame`, `RotationMode`, `RotationMode_LastFrame`.
- Parkour / IK: `ParkourState`, `ParkourLogicalState`, `ParkourAction`, `ClimbStyle`, `ClimbDirection`, `ParkourIK_Enabled`, `LegIK`, `IsHidingOnBeam`, `ClimbMovement`, `LeftHandLedgeLocation`, `RightHandLedgeLocation`, `LeftHandLedgeRotation`, `RightHandLedgeRotation`, `LeftFootLocation`, `RightFootLocation`, `bLeftClimbIK`, `bRightClimbIK`.
- Motion Matching / trajectory: `MMDatabaseLOD`, `CurrentSelectedDatabase`, `CurrentDatabaseTags`, `Trajectory`, `TrajectoryCollision`, `FutureVelocity`, `PreviousDesiredControllerYaw`, `HeavyLandSpeedThreshold`.

### Enum/state ownership (no Blueprint writes)
- `MovementMode`/`MovementMode_LastFrame` come from CharacterMovement in C++ each tick; all Blueprint Set nodes for these must be removed. Recreate `Switch on EMovementMode` after refresh if it turns wildcard.
- `MovementState`, `Gait`, `Stance`, `RotationMode` are C++-written gameplay tags (placeholders until richer data arrives). Blueprints should only read them; delete any Set nodes even if they currently wrote empty tags.
- Parkour tag enums (`ESOTS_ParkourState`, logical/action/climb/direction tags) are sourced from `USOTS_ParkourComponent` via C++; do not write them in Blueprint.

If you see additional `Set` nodes on these names (for example inside reroute-heavy sections of `UpdateEssentialValues`/`UpdateStates`), delete them; the parkour component populates them every tick.

## Fix enum wildcard errors
If any `Equal (Enum)` or `Switch` nodes turn wildcard after reparenting, recreate them by dragging from the corresponding C++ property:
- `MovementMode` / `MovementMode_LastFrame` → `EMovementMode`.
- Other locomotion enums (gait/stance/rotation mode) stay as gameplay tags for now.

## Fix Property Access errors
If Property Access nodes complain about missing bindings (e.g., `Gait`, `CurrentDatabaseTags`, `FutureVelocity`, IK targets), rebind them to the inherited C++ properties with the same names. Delete any Property Access that only existed to feed Set nodes you removed.

## Compile and sanity check
1. Compile the AnimBP; it should show **zero** errors about unwritable fields.
2. In PIE, watch the Anim Instance debug panel:
   - Velocity/Acceleration/Speed2D update while moving.
   - MovementMode/MovementState tags change as you move/jump/fall.
   - Parkour tags and IK targets update when parkour triggers.
   - MM placeholders (`Trajectory`, `FutureVelocity`, `CurrentDatabaseTags`) tick without BP writes.

## Notes
- All fields listed above are `BlueprintReadOnly`; do not flip them to `BlueprintReadWrite` in BP.
- Parkour tags come from `USOTS_ParkourComponent`; Motion Matching placeholders are stubbed in C++ until the full GASP bridge lands.
- If a pin expects data not supplied by C++ (e.g., design-time PoseSearch assets), keep those as BP-only data assets, but do **not** write back into the C++ variables.
