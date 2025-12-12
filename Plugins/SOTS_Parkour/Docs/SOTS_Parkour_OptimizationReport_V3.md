# SOTS_Parkour Optimization Report (V3)

## Overview
SOTS_Parkour is the C++ parity port of the original CGF Parkour Blueprint, responsible for parkour detection, classification, IK targets, and execution for playable characters. The system now runs at full CGF parity (165/165 mapped snippets) and feeds both gameplay and AnimBP interfaces while leveraging OmniTrace for probes.
This document focuses solely on optimization and tuning opportunities. No behavior changes were made in this pass; findings below highlight potential hotspots, trace/IK costs, and future tuning knobs.

## Current Architecture Snapshot (Performance-Relevant Only)
- Tick-driven: `USOTS_ParkourComponent::TickComponent` [Public/SOTS_ParkourComponent.cpp#L540-L620](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L540-L620) runs every frame: state machine, trace stats reset, ledge shimmy input, climb trace cooldown, optional continuous detection.
- Detection entrypoints: `TryPerformParkour`, `TryDetectParkourOnce`, `PerformParkourDetection` [Private/SOTS_ParkourComponent.cpp#L1180-L1325](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L1180-L1325) – build forward capsule traces, optional wall grids, classify actions, update tags/IK.
- Classification: `SelectParkourActionFromHit` and helpers `RunDropHangProbes`, `TryTicTacSideProbes`, `RunWallGridProbes` [Private/SOTS_ParkourComponent.cpp#L1380-L1560](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L1380-L1560) – branching for mantle/vault/tic-tac/drop-hang/climb.
- IK computation: `ComputeClimbIKTargets`, `ComputeHandIKProbes`, `ComputeFootIKProbes` invoked inside `FinalizeResultTags` [Private/SOTS_ParkourComponent.cpp#L900-L1020](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L900-L1020) on every result update.
- ABP/data push: `PushParkourDataToAnimInstance`/`PushParkourStateToAnimInstance` [Private/SOTS_ParkourComponent.cpp#L940-L1010](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L940-L1010) called per result/state change.
- Continuous camera/locomotion helpers: ledge shimmy driver `TickLedgeShimmy` [Private/SOTS_ParkourComponent.cpp#L300-L370](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L300-L370); camera timeline (not read here) triggered on state changes.
- Inputs and tags: BP-provided movement inputs cached (`SetDesiredInputDirection`, etc.) [Public/SOTS_ParkourComponent.h#L170-L260](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L170-L260); gameplay tags updated per result/state.

## Potential Hotspots (Ranked)
1) **PerformParkourDetection** [Private/SOTS_ParkourComponent.cpp#L1180-L1325](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L1180-L1325) – per tick (continuous mode) or per input; builds capsule traces, optional wall grid, speed/camera gates; consumes trace budget and sets cooldowns.
2) **RunWallGridProbes / classification helpers** [Private/SOTS_ParkourComponent.cpp#L1439-L1510](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L1439-L1510) – multiple lateral/vertical traces per attempt, gated by `CanConsumeTrace(2)` but still potentially heavy when grid overlap enabled.
3) **TryTicTacSideProbes / RunDropHangProbes** [Private/SOTS_ParkourComponent.cpp#L1458-L1508](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L1458-L1508) – additional trace clusters triggered for braced distance/tic-tac/drop-hang paths; guarded by `bTriedTicTacDropHangThisFrame` but still per-attempt burst.
4) **IK probe computation** via `FinalizeResultTags` calling `ComputeHandIKProbes` and `ComputeFootIKProbes` [Private/SOTS_ParkourComponent.cpp#L900-L1020](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L900-L1020) every time a result is produced; includes multiple sphere sweeps per limb with miss cooldowns.
5) **TickComponent baseline cost** [Private/SOTS_ParkourComponent.cpp#L540-L620](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L540-L620) – always runs; performs shimmy checks, cooldown decrement, optional continuous detection (potentially heavy if enabled by config).
6) **PushParkourDataToAnimInstance** [Private/SOTS_ParkourComponent.cpp#L940-L985](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L940-L985) – reflection/interface calls to AnimBP per result/state, updating many fields; cost scales with result churn.
7) **Back-hop validation** `CheckBackHopSurfaceClear` and related traces [Public/SOTS_ParkourComponent.h#L120-L210](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L120-L210) (definitions not shown) – capsule sweeps to validate landing; invoked from detection branches.
8) **PredictJump landing probes** [Private/SOTS_ParkourComponent.cpp#L1326-L1379](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L1326-L1379) – forward + downward sphere traces; driven when PredictJump path active.
9) **Ledge shimmy movement** [Private/SOTS_ParkourComponent.cpp#L300-L370](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L300-L370) – runs every tick in climb states, computes tangents/dot products and issues movement input.
10) **Config application/logging** [Private/SOTS_ParkourComponent.cpp#L430-L560](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L430-L560) – one-time, but contains verbose logging; negligible at runtime.

## Tracing & OmniTrace Usage
- All detection traces funnel through `PerformParkourDetection`: one capsule sweep per attempt (ground or climb profile). Uses `ParkourTraceChannel`; budgets enforced by `MaxTracesPerFrame` and `TryConsumeTraceSlot`.
- Climb state throttled by randomized cooldown `ClimbTraceCooldownMin/Max` [Public/SOTS_ParkourComponent.h#L110-L140](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L110-L140); continuous mode bypasses input gating but still respects budgets.
- Wall grid probes (`RunWallGridProbes`) add lateral/vertical sweeps (counts: Horizontal half-quantity * 2, Vertical quantity) controlled by config values `HorizontalWallTraceHalfQuantity_*`, `VerticalWallTraceQuantity_*`, `HorizontalWallTraceRange`, `VerticalWallTraceRange`, and overlap toggle `bUseWallGridOverlap` [Public/SOTS_ParkourComponent.h#L70-L130](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L70-L130).
- Tic-tac and drop-hang probes (`TryTicTacSideProbes`, `RunDropHangProbes`) fire only when XY distance gates are met and guarded by `bTriedTicTacDropHangThisFrame` [Private/SOTS_ParkourComponent.cpp#L1458-L1545](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L1458-L1545).
- Back-hop uses dedicated sweeps and optional surface capsule validation controlled by back-hop config block [Public/SOTS_ParkourComponent.h#L120-L210](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L120-L210).
- IK probes use sphere sweeps: hand probes (per hand) with lift/offset/depth [Public/SOTS_ParkourComponent.h#L200-L320](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L200-L320); foot probes per foot [Public/SOTS_ParkourComponent.h#L320-L400](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L320-L400); miss cooldowns and failure budgets configured to skip traces after repeated misses.
- Budgeting: `MaxTracesPerFrame`, `TraceStats`, `TryConsumeTraceSlot`, `CanConsumeTrace` gate trace bursts; cooldown for climb trace reduces frequency mid-climb.
- Early outs: speed/camera distance gate in `PerformParkourDetection`; character/owner null checks; state gating for shimmy and climb cooldown.

## IK and Animation-Adjacent Cost
- Main IK functions: `ComputeHandIKProbes`, `ComputeFootIKProbes`, `ComputeClimbIKTargets` [Public/SOTS_ParkourComponent.h#L360-L440](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L360-L440) called inside `FinalizeResultTags` [Private/SOTS_ParkourComponent.cpp#L900-L1020](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Private/SOTS_ParkourComponent.cpp#L900-L1020).
- Hand IK: per-hand sphere sweep with lift and depth; gating via `HandIKMaxConsecutiveMissFrames`, `HandIKMissCooldownFrames`, `HandIKMaxConsecutiveFailures` to skip after repeated misses [Public/SOTS_ParkourComponent.h#L220-L300](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L220-L300).
- Foot IK: per-foot sweep with offsets and optional corner-move radius variant [Public/SOTS_ParkourComponent.h#L300-L360](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L300-L360); invoked each result update (i.e., detection success or continuous mode updates).
- Overkill risks: IK traces run on every result even when state/action might not need IK (e.g., idle, drop). Miss cooldown helps but still per-result overhead.
- High-level IK cost reduction ideas (for later implementation):
  - Add state-based gating to skip hand/foot IK when action does not need contact (e.g., drop, predictive jump).
  - Introduce distance/LOD gate (camera distance or pawn size) to halve probe frequency on low-end.
  - Reduce probe count by alternating limbs per frame or decimating when `HandIKCooldownFramesRemaining` is already active.

## Config & Tuning Hooks
Key tunables in `USOTS_ParkourConfig` [Public/SOTS_ParkourConfig.h#L20-L220](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourConfig.h#L20-L220) and mirrored on the component:
- Trace frequency/budget: `bContinuousTraceMode`, `MaxTracesPerFrame`, `ClimbTraceCooldownMin/Max` (lowering reduces trace cost; gameplay risk low-to-medium if detection becomes sluggish mid-climb).
- Detection gates: `MinSpeedForDetection`, `MaxCameraDistanceForDetection` (higher thresholds reduce work but may miss near-idle parkour; risk medium).
- Grid density: `HorizontalWallTraceHalfQuantity_*`, `VerticalWallTraceQuantity_*`, `HorizontalWallTraceRange`, `VerticalWallTraceRange`, `bUseWallGridOverlap` (lower quantities/enable overlap can cut sweeps; risk medium-high—could miss thin ledges).
- Ledge move/hop probes: `LedgeMoveHorizontalDistance`, `LedgeMoveProbeRadius`, `HopVerticalLift` (smaller radius/distance reduces sweep volume; risk medium for detection accuracy).
- Back-hop probes: distances, radii, yaw thresholds, validation capsule sizes [Public/SOTS_ParkourComponent.h#L120-L210](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L120-L210) (lower distances reduce cost; risk medium for hop availability).
- IK probes: `HandIKProbeRadius`, `HandIKTraceDepth`, `HandIKMaxConsecutiveMissFrames`, `HandIKMissCooldownFrames`, `FootIKProbeRadius`, `FootIKTraceDepth`, interp speeds [Public/SOTS_ParkourComponent.h#L200-L360](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourComponent.h#L200-L360) (increasing miss cooldown or reducing depth lowers cost; risk low-to-medium on IK stability).
- Predictive jump probes: `PredictJumpForwardDistance`, `PredictJumpProbeDepth`, `PredictJumpProbeRadius` [Public/SOTS_ParkourConfig.h#L170-L205](Plugins/SOTS_Parkour/Source/SOTS_Parkour/Public/SOTS_ParkourConfig.h#L170-L205) (smaller values reduce sweep volume; risk medium).
- Linger timings: `Linger*Seconds` affect state machine duration but not trace cost (risk low for perf, gameplay feel impact medium).
Missing knobs to consider later:
- Explicit per-state IK enable/disable flags.
- Per-action trace budgets (e.g., limit wall grid only for certain actions).
- Camera-distance-based IK/trace LOD multiplier.

## Safe vs Risky Optimization Suggestions
**Low-risk / Safe**
- Cache and reuse derived vectors (surface normal/tangent) in `TickLedgeShimmy` and detection to avoid repeated normalizations when unchanged per frame.
- Tighten early-outs in `PerformParkourDetection` by checking `TryConsumeTraceSlot` before expensive grid probes or secondary probes even when `CanConsumeTrace` already passes.
- Skip `PushParkourDataToAnimInstance` fields that did not change (state/action tag churn) to reduce interface call volume.
- Ensure debug logging (`bEnableDebugLogging`/`bEnableTraceDebug`) is off in production; keep verbose logs guarded.

**Medium/High-risk**
- Reduce wall grid density (`HorizontalWallTraceHalfQuantity_*`, `VerticalWallTraceQuantity_*`) or increase step ranges to cut sweep count—may miss narrow ledges; playtest traversal reliability.
- Increase climb trace cooldown and/or disable `bContinuousTraceMode` by default to lower per-frame detection in climb; could delay reactions on fast inputs.
- Add state-based IK gating (skip hand/foot IK for drop/predictive jump) or halve IK frequency; risk: hands/feet may stick during transitions—needs animation review.
- Lower IK trace depths/radii and raise miss cooldown frames to reduce per-frame sweeps; risk: IK snapping/float on uneven surfaces.
- Introduce camera-distance LOD that disables detection/IK beyond a threshold; risk: distant spectators lose parkour fidelity.

## Debug & Instrumentation Ideas
- Counters for traces per category: first capsule, wall grid, tic-tac/drop-hang, IK hand/foot per frame/state.
- Timers around `PerformParkourDetection`, `RunWallGridProbes`, `ComputeHandIKProbes`, `ComputeFootIKProbes` to profile spikes.
- Histogram of `ClimbTraceCooldownTime` usage to see effective sampling rate in climb.
- Optional on-screen stats widget (extending `ISOTS_ParkourStatsWidgetInterface`) showing trace budget usage and IK skips.

## Summary & Priority List
Biggest wins likely come from reducing trace volume (wall grids, climb cooldown) and gating IK work when not needed. IK probes and wall grids are the primary cost multipliers; state-based gating and LOD-style cooldowns can trim cost with controlled risk.

Top 5 opportunities (impact vs risk):
1. Add/adjust climb trace cooldown and disable continuous detection in non-debug builds to cut per-frame sweeps (low-medium risk).
2. Reduce wall grid density or make it state/action-gated to shrink burst trace counts (medium risk).
3. Introduce state-based IK gating or per-frame decimation for hand/foot probes (medium risk).
4. Increase miss cooldowns and failure thresholds for hand/foot IK to skip repeated failing traces (low-medium risk).
5. Cache/tag-change detection to avoid redundant AnimBP interface calls and repeated math in shimmy/detection paths (low risk).
