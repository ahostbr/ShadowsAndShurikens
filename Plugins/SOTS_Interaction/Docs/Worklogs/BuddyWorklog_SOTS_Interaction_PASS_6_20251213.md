# Buddy Worklog - SOTS_Interaction - PASS 6 (OmniTrace Standardization)

## What changed
- Added an internal trace wrapper that can route through OmniTrace when available, with fallback to Engine traces.
- Replaced subsystem sweep/LOS calls with the wrapper to preserve behavior while enabling suite-standard tracing.

## Files added
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionTrace.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionTrace.cpp

## Files changed
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp

## Notes / Assumptions
- OmniTrace include/entry points are gated with __has_include and TODO markers for the suite-specific API.
- Debug drawing remains compile-guarded out of Shipping/Test; behavior matches prior sweeps/LOS.

## TODO / Next
- Wire real OmniTrace sweep/line trace calls once the suite surface is confirmed.
- Continue with Interaction Driver (pass 7) and fail reason polish (pass 8).