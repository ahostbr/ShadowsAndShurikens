# SOTS_AIPerception SPINE3 Event Contract — 2025-12-14 20:57:40

## Telemetry payload
- Struct: `FSOTS_AIPerceptionTelemetry`
  - Suspicion01 (clamped [0,1])
  - bDetected
  - ReasonSenseTag (per-sense with generic fallback)
  - LastStimulusWorldLocation
  - LastStimulusActor (weak)
  - LastStimulusTimeSeconds
  Evidence: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1908-L1921](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1908-L1921)

## Delegates and broadcasts
- BlueprintAssignable delegates: OnAIPerceptionSpotted/Lost/SuspicionChanged declared on component.
  Evidence: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L92-L129](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L92-L129)
- Broadcast sites: suspicion/telemetry dispatch in UpdateSuspicionModel.
  Evidence: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1822-L1891](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1822-L1891) and telemetry broadcast helper [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1922-L1951](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1922-L1951)

## Blueprint helper surface
- Component BlueprintPure helpers: suspicion/detected/reason/last stimulus accessors.
  Evidence: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L60-L82](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L60-L82)
- Suspicion computation and clamping for helpers: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1779-L1865](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1779-L1865)

## Diagnostics
- Dev-only event logs (default OFF): `bDebugLogAIPerceptionEvents` in GuardConfig drives UE_LOG for telemetry at broadcast time. Evidence: [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1936-L1951](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1936-L1951)

## Notes
- No .bak or duplicate GSM/event wiring found.
- Builds/tests not run per instruction.
- Binaries/Intermediate cleaned for plugin (see cleanup step).
