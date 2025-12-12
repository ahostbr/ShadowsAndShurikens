# SOTS_Parkour – Documentation

## 1. Overview

`USOTS_ParkourComponent` is the C++ port of the legacy CGF Parkour Blueprint. It owns parkour detection (traces), classification (mantle/vault/drop/hop), and execution (motion warp or teleport fallback) for the player character. The goal is to maintain 1:1 parity with the old Blueprint while staying designer-tunable.

## 2. Docs Index

- `SOTS_Parkour_Parity_CGF.md` — detailed parity report vs CGF Parkour BP.
- `SOTS_Parkour_Debug_Validation.md` — debug flags and in-editor validation steps.

## 3. Key Public Knobs (by Category)

- `SOTS|Parkour|Debug`: `bEnableTraceDebug`, `bEnableDebugLogging`, `TraceDebugDrawTime`, per-feature debug draw times.
- `SOTS|Parkour|Trace`: primary detection ranges (vault/min/max, ledge move distances, back-hop distances, wall trace grids).
- `SOTS|Parkour|LOD`: speed and camera distance gates.
- `SOTS|Parkour|IK`: climb hand spacing/offsets.
- `SOTS|Parkour|Execution|Warp`: motion-warp authoring height and target name.
- `SOTS|Parkour|Config`: optional DataAsset and action set references.
- `SOTS|Parkour|Bridge` / `UI` / `Camera`: bridge events, camera timeline knobs, optional stats widget.

## 4. Assets & Config

- Optional `USOTS_ParkourConfig` DataAsset: holds detection/classification thresholds and trace profile map.
- Optional `USOTS_ParkourActionSet` or DataTable: action definitions and warp targets.
- Debug/QA guidance: see `Docs/SOTS_Parkour_Debug_Validation.md`.

## 5. Integrating on a Character

1. Add `USOTS_ParkourComponent` to the character (Blueprint or C++); set `ParkourConfig` if available.
2. Ensure movement input is forwarded via `SetDesiredInputDirection` / `SetMovementSpeed2D` or let it derive from velocity.
3. Drive parkour attempts with `TryPerformParkour` (or `TryDetectParkourOnce` for detection-only use).
4. Use `SetParkourDebugEnabled` to toggle traces/logging during QA.
5. For animation/bridge use, bind to `OnParkourActionStarted/Ended` and read `GetLastResult()` for the latest decision.

## 6. Parity & Debug

- Parity status and numeric defaults: `Docs/SOTS_Parkour_Parity_CGF.md`.
- Debug/validation steps: `Docs/SOTS_Parkour_Debug_Validation.md`.
