# SOTS_KEM Completeness Pass — SPINE 4 (Height Tuning & Coverage Validation)

## Summary
- Clarified height gating semantics with explicit min/max vertical fields and optional dev logging for height rejections (backward compatible defaults).
- Added an editor-only registry coverage validator to flag missing execution families/position tags.
- Hygiene: no stale .bak files found in KEM; none removed.

## New config fields (defaults)
- `bDebugLogHeightRejections=false` (`SOTS_KEMManagerSubsystem`): dev-only verbose logs when height gating rejects a candidate.
- Height fields already present, kept for back-compat:
  - `MaxSamePlaneHeightDelta=15`
  - `MinVerticalHeightDelta=15` (defaults to prior same-plane value for unchanged behavior)
  - `MaxVerticalHeightDelta=0` (0 = no ceiling)

## Height gating rules (unchanged defaults)
- SamePlaneOnly: `abs(HeightDelta) <= MaxSamePlaneHeightDelta`
- VerticalOnly: `abs(HeightDelta) >= MinVerticalHeightDelta` and (`MaxVerticalHeightDelta == 0` or `abs(HeightDelta) <= MaxVerticalHeightDelta`)
- Any: no height gate
- Dev log on reject if `bDebugLogHeightRejections` and non-shipping/test.

## Editor-only validator
- Function: `KEM_ValidateRegistryCoverage(WorldContextObject, OutWarnings)` (Blueprint callable; editor-only).
- Scans loaded definitions from subsystem, catalog, and default registry config; warns for missing execution families (Front/Rear/Left/Right, CornerLeft/CornerRight, VerticalAbove/VerticalBelow) and position tags (Ground.*, Corner.*, Vertical.*), plus missing fallback (ExecutionFamily Unknown).
- Outputs warnings array and logs warnings in editor.

## Evidence pointers
- Height rejection logging hooks: `EvaluateTagsAndHeight`, `EvaluateCASDefinition` in `SOTS_KEM_ManagerSubsystem.cpp`.
- Config flag: `bDebugLogHeightRejections` in `SOTS_KEM_ManagerSubsystem.h`.
- Coverage validator: `KEM_ValidateRegistryCoverage` in `SOTS_KEM_BlueprintLibrary.cpp` (decl in header).

## Behavior impact
- Defaults preserve existing gameplay; new logging/validation are opt-in and editor/dev-only.

## Hygiene
- No KEM-local .bak/old files present; none deleted.
