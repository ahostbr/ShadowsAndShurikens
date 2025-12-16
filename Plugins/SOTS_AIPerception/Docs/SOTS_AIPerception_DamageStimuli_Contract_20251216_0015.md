# SOTS AIPerception Damage Stimuli (Selectors First)

- Pipeline: Engine damage event (`OnTakeAnyDamage` on the victim) -> `USOTS_AIPerceptionComponent::ApplyDamageStimulus` -> GSM `ReportAISuspicionEx` (with instigator + optional location). Manual path: `SOTS_TryReportDamageStimulus` (Library) -> `USOTS_AIPerceptionSubsystem::TryReportDamageStimulus` -> victim component `ApplyDamageStimulus` -> GSM.
- Policy resolution: lookup `DamageTag` in `DamagePolicies`; fallback to `DefaultDamagePolicy` when tag missing/empty. Relation filters (self/friendly/neutral/unknown) follow policy booleans; ignored stimuli are silently dropped.
- Cooldown/range: per-tag cooldown via `NextAllowedDamageTimeByTag`; optional `MaxRange` gate (uses instigator location if present, else provided location).
- Severity scaling: `Impulse = SuspicionImpulse * clamp(DamageAmount / max(SeverityRefDamage, eps), 0..MaxSeverityScale)` when `bScaleByDamageAmount` is true. Impulse is applied as normalized suspicion delta and clamped to [0..1].
- Escalation: optional `bForceMinPerceptionState` forces at least `MinPerceptionState` (ESOTS_PerceptionState index).
- Location resolution: prefers provided location; else instigator actor location; if neither, GSM is called without a location. Last stimulus cache stores best-effort location and timestamp.
- GSM reason tag: `OverrideReasonTag` if set, else `GuardConfig.ReasonTag_Damage`, else generic. `bAlwaysReportToGSM` forces a report even if suspicion delta is small.
- OnTakeAnyDamage default tag: the engine hook does not supply `DamageTag`; it uses `DefaultDamagePolicy`. Use `SOTS_TryReportDamageStimulus` for tagged/cutscene/poison events.
