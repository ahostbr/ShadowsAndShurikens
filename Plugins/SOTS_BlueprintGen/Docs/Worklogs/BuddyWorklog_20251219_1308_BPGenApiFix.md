# Buddy Worklog — 2025-12-19 13:08 — BPGen API Fix

## Goal
- Unblock SOTS_BlueprintGen compile errors (TryGetObjectField signature changes, missing result fields, helper symbols).

## What changed
- Updated BPGen API JSON parsing to UE5.7 TryGetObjectField out-param usage for options/params.
- Expanded BPGen result structs (GraphEditResult, AssetResult) with ErrorMessage/Errors/Warnings to match envelope expectations.
- Added missing helper declarations/definitions (pin type parsing, default object assignment, container parsing, pin direction text) and UE5.7 variable spawner adaptation; wired CVar for link fallback.
- Included knot/select node headers and implemented EnsureBlueprintNodeModulesLoaded loader.

## Files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenAPI.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h

## Notes / Risks / Unknowns
- Variable spawner matching now uses VarProperty/owner; behavior parity with prior API not yet validated.
- Pin type conversion is minimal; may need richer mapping for complex graph specs.
- Build artifacts removed for SOTS_BlueprintGen per policy; rebuild required.

## Verification
- NOT VERIFIED (no build/run executed here).

## Follow-ups / Next steps
- Re-run UBT to confirm compile passes and surface any remaining errors.
- Adjust pin type mapping or variable spawner matching if runtime results differ.
