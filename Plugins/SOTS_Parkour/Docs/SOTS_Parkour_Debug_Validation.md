# SOTS Parkour – Debug & Validation Guide

## 1. Purpose

Quick steps to visually validate `USOTS_ParkourComponent` behavior against the legacy CGF Parkour Blueprint using in-editor tools and unified debug flags.

## 2. Setup

- Use a lightweight test level that includes:
  - Low vault obstacles (≈15–55uu).
  - Standard ledges for mantles and lateral ledge moves.
  - Spots with open space behind walls for back-hop.
  - Inner/outer corners for corner moves.
- Ensure the player Character owns `USOTS_ParkourComponent` with the same config used in the parity doc (`SOTS_Parkour_Parity_CGF.md`).

## 2.1 Prerequisites

- Character must have `USOTS_ParkourComponent` added and active.
- A parkour config asset (if used) should match the values in `SOTS_Parkour_Parity_CGF.md`.
- Optional: bind a simple debug widget or console alias to call `SetParkourDebugEnabled`.

## 3. Enabling Debug

- On the player’s `USOTS_ParkourComponent` call:
  - `SetParkourDebugEnabled(true, true)` to enable both trace visuals and verbose logging.
  - `SetParkourDebugEnabled(true, false)` for visuals only.
  - `SetParkourDebugEnabled(false, true)` for logs only.
- Trace Debug shows:
  - Forward/confirm capsule traces (first capsule, mantle/vault confirm).
  - Ledge move / back-hop / tic-tac sphere traces (angle-gated for vertical walls).
  - Hand/foot IK traces and climb probes.
- Logging Debug shows:
  - Parkour state/logical state transitions.
  - Detected action summaries (mantle/vault/drop/back-hop/tic-tac/corner move).
  - Trace summaries from `LogTraceResult` when traces fire.

## 4. Visual Checks

### 4.1 Forward Capsule Trace
- Walk and sprint toward a wall.
- Verify start Z offset changes when falling vs grounded; forward distance scales with speed.
- Confirm grid refinement snaps to the best height/XY when wall probes are enabled.

### 4.2 Mantle & Vault Surfaces
- Approach low (15–55uu) obstacles: expect vault.
- Approach mid-height obstacles: expect mantle (respecting Min/Max mantle heights).
- Watch confirm trace capsules for start/end positions and impact point markers.

### 4.3 Back-Hop & Ledge Moves
- Stand at a ledge, face the wall, trigger parkour:
  - Back-hop should respect the 90° ± tolerance wall check.
  - Landing distance should match inner (55uu) then primary (140uu) probes.
- Lateral input along ledge should produce `LedgeMove` with side sphere traces visible.

### 4.4 Corner Moves
- At inner/outer corners, trigger parkour while pressing toward the corner.
- Confirm `MoveComponentTo` path feels smooth; debug traces should show the forward/down probes used to pick the target.

### 4.5 Tic-Tac Side Probes
- Approach a wall at speed and angle; enable trace debug.
- Side probes should show short lateral traces; accepted hits should align with tic-tac launches.

### 4.6 IK / Hand & Foot Placement
- Hang on a ledge:
  - Hand traces (orange/yellow) should respect spacing, forward/back offsets, and normal dot threshold.
  - Foot traces (green/cyan) should land on believable surfaces; corner moves use the wider probe radius.

## 5. Troubleshooting

- **No traces drawing:**
  - Ensure `SetParkourDebugEnabled(true, false)` was called at least once.
  - Confirm the component is Idle/NotBusy and parkour detection is allowed.
- **Logs too noisy:**
  - Switch to `SetParkourDebugEnabled(true, false)` to keep visuals only.
- **Behavior mismatch vs CGF:**
  - Cross-check defaults in `SOTS_Parkour_Parity_CGF.md` (Sections 1–6).
  - Note any edge cases for the next parity logic pass.

## 6. Quick Checklist

- Forward capsule traces scale with speed and show correct start Z.
- Mantle/vault confirm traces appear with expected radii/offsets.
- Back-hop uses inner then primary distance; angle gate respected.
- Ledge move and tic-tac side probes render when attempted.
- IK hand/foot traces appear only when relevant states are active.

Use this guide during in-editor play to quickly spot regressions and verify parity fixes as they land.
