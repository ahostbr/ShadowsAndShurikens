# SOTS AIPerception Noise Stimuli Contract (2025-12-15)

## Function signatures
- Library: `SOTS_TryReportNoise(WorldContext, Instigator, Location, Loudness, NoiseTag, bLogIfFailed=false)` (BP-callable). Deprecated wrapper `SOTS_ReportNoise(...)` forwards.
- Subsystem: `TryReportNoise(AActor* Instigator, const FVector& Location, float Loudness, FGameplayTag NoiseTag)` → broadcasts to registered components preserving instigator/tag.
- Component: `HandleReportedNoise(const FVector& Location, float Loudness, AActor* InstigatorActor, FGameplayTag NoiseTag)` applies policy + suspicion + GSM.

## Relation filtering
- Relation from `InstigatorActor` vs owner: Self, Friendly, Neutral, Hostile, Unknown (team-aware via `IGenericTeamAgentInterface`).
- Config ignores: `bIgnoreSelfNoise`, `bIgnoreFriendlyNoise`, `bIgnoreNeutralNoise`, `bIgnoreUnknownNoise`.

## Noise policy resolution
- `NoisePolicies` (tag → `FSOTS_NoiseStimulusPolicy`) checked when `NoiseTag` valid; otherwise `DefaultNoisePolicy`.
- Policy fields: `SuspicionDelta`, `MaxRange` (0 disables), `CooldownSeconds` (per-tag), `LoudnessScale`, `OverrideReasonTag` (fallback to Hearing/Generic tag).

## Cooldown & range
- Range gate: if `MaxRange > 0`, compare owner↔instigator (or noise location if no instigator); drop when beyond range.
- Cooldown: `NextAllowedNoiseTimeByTag` map; if `Now < NextAllowed` drop; otherwise set `Now + CooldownSeconds`.

## Application & GSM reporting
- Impulse: `SuspicionDelta * Loudness * LoudnessScale` added to normalized suspicion (clamped 0..1), updates guard suspicion state and last stimulus cache.
- Pending GSM payload: ReasonTag (override/hearing/generic), Location (noise), InstigatorActor. Sends via GSM `ReportAISuspicionEx` (force report) with ReasonTag + Location + Instigator.

## Logging
- VeryVerbose logs (gated by perception telemetry/debug) for relation/range/cooldown drops and applied deltas.
