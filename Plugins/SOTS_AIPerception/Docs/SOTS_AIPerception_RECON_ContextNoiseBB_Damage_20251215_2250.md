# SOTS AIPerception Recon — Context/Noise/BB/Damage (2025-12-15)

Scope: map current behavior for context-rich reporting, noise ingestion, blackboard writes, and damage stimulus wiring. No code changes yet.

## Key Findings
- **Noise path**: NoiseTag/Instigator are dropped end-to-end. Library → subsystem forwards only `Location`/`Loudness`; component `HandleReportedNoise` ignores tag/instigator and only bumps hearing for the primary target. No distance falloff to GSM beyond a forced report. See [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionLibrary.h](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionLibrary.h), [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionSubsystem.h](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionSubsystem.h), [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionSubsystem.cpp](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionSubsystem.cpp), [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp).
- **Context/telemetry**: `FSOTS_LastPerceptionStimulus` caches only for the primary target and overwrites by strongest timestamped stimulus. GSM reporting only receives suspicion01; reason tags/locations are not forwarded. Telemetry delegates emit ReasonSenseTag but no instigator info. MapSenseToReasonTag respects configured ReasonTag_* but damage slot is unused. See [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h) and component file.
- **Blackboard writes (dual paths)**: Perception tick writes BB twice: (1) config-driven keys (Suspicion/State/HasLOS) inside `UpdatePerception`, and (2) hard-coded `TargetActor`/`HasLOSToTarget`/`LastKnownTargetLocation`/`Awareness`/`PerceptionState` inside `UpdateBlackboardAndTags`. Both run every tick, so key drift is possible if config keys overlap defaults. See component file.
- **GSM bridge**: `ReportSuspicionToGSM` throttles and sends only a float; ReasonTag/location are calculated but never passed into GSM API (GSM has no params for them). GSM aggregates per-guard suspicion via max; reasons are lost. See component file and [Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h](Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h).
- **Damage stimulus**: Config exposes `RampUpPerSecond_Damage` and `ReasonTag_Damage` but no damage ingestion path exists. No OnTakeDamage binding; UpdateSuspicionModel handles Damage sense only if invoked manually. No LastStimulus updates for damage.
- **Hearing → suspicion**: Noise sets `PendingHearingStrength` and forces GSM report with hearing reason tag. Suspicion model applies ramp and ReasonTag_Hearing, but instigator/tags are not stored and LastStimulus caches only if target is the resolved primary actor.
- **Primary-target assumption**: Many paths (`ResolvePrimaryTarget`, noise handling, LastStimulus cache) assume a single primary actor (player). If unresolved, noise and tag updates are dropped.

## Gaps vs Targets
- **1C context-rich reporting**: No instigator, NoiseTag, or full stimulus context reach GSM; telemetry lacks instigator. LastStimulus only for primary target.
- **2D noise sense improvements**: NoiseTag unused; no spatial filtering beyond radius; no per-tag routing; no LOS occlusion or noise class weighting.
- **3C blackboard dual-mode**: Both BB writers run; no guardrails ensuring config selectors are the ones used in BTs; hard-coded keys always written.
- **4C damage stimulus model**: No damage hook; Damage ramp/tag never used; GSM never receives damage-driven reason/location.

## Suggested next steps
1) Add structured noise payload (tag, instigator, location, loudness) to subsystem/component and optionally GSM. Gate by primary-target ownership and distance/visibility if needed.
2) Single-source BB writes: use config selectors only, or map hard-coded keys into config to avoid duplication.
3) Extend GSM API to accept reason/location; have component forward LastStimulus info with instigator.
4) Implement damage ingestion (OnTakeAnyDamage/point/radial) into suspicion ramp using RampUpPerSecond_Damage and ReasonTag_Damage.
