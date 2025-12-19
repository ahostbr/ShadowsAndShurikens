# SOTS_AIPerception SPINE6 Shipping Sweep (20251215_0215)

## Guard changes checklist
- Suspicion debug print now dev-only, gated by `bDebugSuspicion` and compiled out in Shipping/Test [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1048-L1068].
- Perception telemetry logging wrapped in Shipping/Test guard and still keyed off `bEnablePerceptionTelemetry` [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1545-L1594].
- Telemetry/event logs for perception events now explicitly dev-only (bDebugLogAIPerceptionEvents) [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1958-L1975].
- GSM missing/report spam already config-gated and stays dev-only [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1989-L2064].

## Editor-only leak status
- No WITH_EDITOR/Editor module references after sweep (DevTools regex: WITH_EDITOR|UnrealEd|Editor|AssetRegistry|Blutility → 0 hits).
- Build.cs has runtime-only dependencies (no Editor modules) [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/SOTS_AIPerception.Build.cs#L15-L43].

## TODO / dead-end cleanup
- No TODO/FIXME/placeholder/.bak items remain (DevTools regex sweep yielded 0 hits). No stubs left in callable paths.

## Public API: no silent failure
- Added bool-returning `SOTS_TryReportNoise` for BP/C++ callers; legacy `SOTS_ReportNoise` now delegates and is marked deprecated [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionLibrary.h#L10-L27].
- Library implementation returns success/failure and offers optional dev-only failure log (off by default) [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionLibrary.cpp#L9-L42].
- World subsystem now reports whether any listener consumed the noise (TryReportNoise) while keeping legacy void wrapper [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionSubsystem.cpp#L25-L61].

## SPINE 1–6 recap
- SPINE1 GSM reporting: GSM report cadence, tagging, and logging wired; debug GSM logs remain dev-only [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1991-L2064].
- SPINE2 suspicion model: stable ramp/decay, hysteresis, event throttles, and GSM reason tagging [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1183-L1528].
- SPINE3 event contract + BP helpers: delegates for suspicion/spotted/lost plus telemetry snapshot builder [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1878-L1956].
- SPINE4 senses/targeting + AIBT compatibility: primary target resolution, LOS/target-point evaluation unchanged; rechecked BEP exports (see below) to ensure function names/outputs match AIBT usage.
- SPINE5 lifecycle/reset: stateless reset reasons (BeginPlay/EndPlay/Mission/Profile), OnUnregister reset, LastResetReason tracking [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L32-L120].
- SPINE6 shipping safety: log categories defined once [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L1-L18] and all debug/telemetry logs gated; noise APIs made non-silent.

## Evidence pointers
- Log category declarations for consistent gating [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L1-L18]; definitions in component cpp [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L13-L18].
- Suspicion debug guard (dev-only) [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1048-L1068].
- Telemetry guard with Shipping/Test strip [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1545-L1594].
- Dev-only perception event logging [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1958-L1975].
- Try-report noise API surface [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionLibrary.cpp#L9-L42] and subsystem delivery return [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionSubsystem.cpp#L25-L61].

## BEP_EXPORTS \ AIBT consulted (confirmation)
- BEP_EXPORTS/AIBT/2025-12-12_09-58-55/Blueprints/BP_BehaviorComponent/Snippets/SetDefaults.snippet.txt (perception component access, perception update events).
- BEP_EXPORTS/AIBT/2025-12-12_09-58-55/Blueprints/BP_AIStorage/Snippets/ProximityOptimization.snippet.txt (AIPerception component queries).
- BEP_EXPORTS/AIBT/2025-12-12_09-58-55/Blueprints/BP_AIC/Snippets/EventGraph.snippet.txt (OnTargetPerceptionUpdated / stimuli handling).

## Notes
- Shipping/Test builds now strip all debug/telemetry logs; all toggles default to false. Legacy APIs preserved via deprecated wrappers to avoid breaking existing BP hookups.
