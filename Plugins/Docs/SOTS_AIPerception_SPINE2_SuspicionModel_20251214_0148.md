# SOTS_AIPerception — SPINE2 Suspicion Model (2025-12-14 01:48)

## Scope
Stabilize suspicion/detection math with clamped 0..1 outputs, configurable ramp/decay, hysteresis and min-duration gates, event throttling, and retained GSM single-path reporting. No builds/tests run.

## Evidence
- Tuning knobs (rates, thresholds, hysteresis, dwell, epsilon/interval, forget delay): [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L120-L160](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L120-L160).
- Centralized suspicion update with clamp [0,1], ramp/decay, dominant sense selection, hysteresis, min-time gates, and event throttle: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1515-L1667](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1515-L1667).
- UpdatePerception now feeds sight/hearing/shadow strengths into UpdateSuspicionModel and keeps GSM reporting single-path: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L695-L846](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L695-L846).
- Hearing stimulus now accumulates strength (no direct suspicion spike) and still force-flags GSM with hearing reason/location: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L484-L541](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L484-L541).

## Behavior changes (defaults preserve feel)
- Ramp/decay defaults mirror prior values: sight 0.25/s, hearing 0.75/s (~0.15 per 0.2s update), shadow 0.12/s, decay 0.1/s, thresholds 0.9/0.85 with hysteresis on.
- Hysteresis/min-time gates prevent flicker; optional dwell/flip limits default OFF.
- Suspicion sanitized/clamped every update; dominant sense stored for GSM reason; detection events broadcast via OnTargetSpotted/OnTargetLost with throttled OnSuspicionChanged (epsilon 0.01, interval 0.10s).

## Notes
- GSM reporting remains single-path (SPINE_1); state changes still force reports.
- No stealth tier tags are written from suspicion; only computation + events + GSM feed.

## Cleanup
- Plugin binaries/intermediate removed after changes.
