# SOTS Interaction — OmniTrace Spine 1

## Summary
- Rewired all interaction traces (candidate sweep and LOS) to use OmniTrace by default with a legacy fallback guard.
- Added dev toggles for interaction trace debug draw/logging and recorded OmniTrace usage on candidate state.
- Ensured subsystem exposes optional legacy flag and OmniTrace dependency is declared.

## New presets / config flags
- `bForceLegacyTraces` (default false) to explicitly use engine traces instead of OmniTrace.
- `bDebugDrawInteractionTraces` (default false) dev-only draw for sweeps/LOS.
- `bDebugLogInteractionTraceHits` (default false) dev-only verbose logging for sweep/LOS hits.

## File changes
- `Plugins/SOTS_Interaction/Source/SOTS_Interaction/SOTS_Interaction.Build.cs`: add `OmniTrace` private dependency.
- `Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionTrace.h/cpp`: route sweep/LOS through OmniTrace with debug draw/log options and legacy guard.
- `Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h` and `Private/SOTS_InteractionSubsystem.cpp`: expose new toggles, pass them into trace helpers, track OmniTrace usage.

## Evidence pointers
- Candidate sweep now OmniTrace-backed with legacy guard: `SOTSInteractionTrace::SphereSweepMulti` and call from `FindBestCandidate` in `SOTS_InteractionSubsystem.cpp`.
- LOS check now OmniTrace-backed with legacy guard: `SOTSInteractionTrace::LineTraceBlocked` and call from `PassesLOS` in `SOTS_InteractionSubsystem.cpp`.
- Legacy fallback explicitly controlled by `bForceLegacyTraces`; defaults keep OmniTrace path active.

## Verification
- Not run (per instructions: no builds/cook/tests).

## Cleanup
- Removed `Plugins/SOTS_Interaction/Binaries/` and `Plugins/SOTS_Interaction/Intermediate/` after edits.
