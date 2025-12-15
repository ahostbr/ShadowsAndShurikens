# SOTS_AIPerception — SPINE1 GSM Wiring (2025-12-14 01:30)

## Goal
Lock the AIPerception → GSM reporting path with throttled AISuspicion01, reason tags, and single-source wiring; remove legacy dead ends.

## Evidence (current state)
- AISuspicion compute and normalized feed now captured once per update with reason accumulation and force flags: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L695-L846](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L695-L846).
- Throttled GSM reporter with interval/delta gates, cached resolver, and reason-aware telemetry: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1515-L1602](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1515-L1602).
- Config defaults for GSM reporting (enabled, 0.15s min interval, 0.01 min delta) plus sense reason tags and dev missing-GSM log toggle: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L119-L148](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L119-L148).
- Hearing events now flag a forced GSM report with hearing reason + location: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L484-L541](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L484-L541).
- Legacy .bak stubs removed: SOTS_AIPerceptionComponent.cpp.bak and SOTS_AIPerception.Build.cs.bak (deleted).

## Behavior changes
- Added `ResolveGSM` caching helper; optional dev log when GSM missing (default OFF).
- Added `ReportSuspicionToGSM` with clamp + min interval (default 0.15s) + min delta (default 0.01) and force flag for state changes or discrete stimuli; retains last reason tag/location for telemetry.
- AISuspicion reporting now routed through the throttled helper instead of per-tick direct GSM call; state changes force a send.
- Sense reason mapping hooks: sight bumps set `ReasonTag_Sight`; shadow bumps set `ReasonTag_Shadow`; hearing bumps force-send with `ReasonTag_Hearing` and noise location; fallback `ReasonTag_Generic` if nothing set.

## Throttle defaults
- Enabled by config (`bEnableGSMReporting=true`).
- `GSMReportMinIntervalSeconds=0.15f`, `GSMReportMinDelta01=0.01f` (configurable per guard data asset).
- Force bypass when perception state changes or a pending forced report is queued (e.g., hearing event).

## Reason tag mapping table
- Sight: `ReasonTag_Sight` (guard config)
- Hearing: `ReasonTag_Hearing` (guard config)
- Shadow awareness: `ReasonTag_Shadow` (guard config)
- Damage: `ReasonTag_Damage` (reserved for future use)
- Fallback: `ReasonTag_Generic` (guard config)

## Legacy cleanup
- Removed stale .bak files to eliminate duplicate/obsolete GSM wiring paths.

## Next steps
- If desired, wire actual gameplay tags into the new reason slots (SOTS tag schema) and tune interval/delta per archetype.
- Validate in editor that suspicion reports land in GSM ingest with reduced spam (no build run per instruction).
