# SOTS_AIPerception SPINE1 GSM Wiring — 2025-12-14 20:56:45

## What changed / verified
- Confirmed a single perception → GSM pipeline: AISuspicion is computed in `UpdateSuspicionModel` and reported once via `ReportSuspicionToGSM` after throttling/gating.
- Verified reason tags are mapped per sense and forwarded with GSM reports; no duplicate/legacy GSM wiring remains (no .bak/TODO reconnect hits).

## Evidence
- AISuspicion01 compute + clamp + hysteresis: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1779-L1865](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1779-L1865)
- Reason tag selection per sense: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1594-L1616](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1594-L1616)
- GSM report call (throttled, clamped, cached GSM resolver): [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1952-L2044](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1952-L2044)
- Throttle defaults (min interval/delta, enable flag, sense reason tags): [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L201-L233](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L201-L233)
- GSM dependency declared (single module): [Plugins/SOTS_AIPerception/SOTS_AIPerception.uplugin#L39-L40](Plugins/SOTS_AIPerception/SOTS_AIPerception.uplugin#L39-L40), [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/SOTS_AIPerception.Build.cs#L39-L41](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/SOTS_AIPerception.Build.cs#L39-L41)
- Legacy checks: no `.bak` or TODO reconnect GSM hits (see ad_hoc_regex_search run in terminal).

## Notes
- Reporting is change-gated by both `GSMReportMinIntervalSeconds` (0.15s default) and `GSMReportMinDelta01` (0.01f default); spotted/lost can force a report via `bForceNextGSMReport`.
- Reason tags prefer configured sense tags with fallback to `ReasonTag_Generic`; telemetry mirrors the same sense tag used for GSM.

## Verification
- No builds/tests run (per instruction).

## Cleanup
- SOTS_AIPerception `Binaries/` and `Intermediate/` were already removed after prior edits; no new binaries generated.
