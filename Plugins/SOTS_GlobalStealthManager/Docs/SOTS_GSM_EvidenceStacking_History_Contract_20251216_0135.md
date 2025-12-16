# SOTS GSM Evidence + History Contract (2025-12-16)

## Rolling history
- Buffer type: `FSOTS_GSM_AISuspicionEvent` (Suspicion01, ReasonTag, bHasLocation, Location, InstigatorActor, TimestampSeconds).
- Per-AI buffer stored on `FSOTS_GSM_AIRecord.RecentEvents`.
- Prune rule: drop events older than `EvidenceConfig.WindowSeconds` (default 6s).
- Bound: clamp to `EvidenceConfig.MaxEventsPerAI` (min 1; default 8) by removing oldest.

## Evidence stacking
- Per-event points: `EvidencePointsPerEvent` (default 1.0) * optional reason multiplier (map lookup) * optional `SameInstigatorMultiplier` if InstigatorActor matches incoming.
- Events with Suspicion01 < `MinSuspicionToCountAsEvidence` (default 0.10) do not contribute.
- Points clamped to `MaxEvidencePoints` (default 6.0).
- Evidence bonus: `(Points / MaxEvidencePoints) * MaxEvidenceSuspicionBonus` (default max bonus 0.25); clamped 0..MaxEvidenceSuspicionBonus.

## Numeric blend
- Prior/current blend: `BlendedBase = Lerp(Incoming, PrevSuspicion, PriorSuspicionBlendAlpha)` (default alpha 0.35).
- Final suspicion: `Final = clamp(max(Incoming, BlendedBase) + EvidenceBonus, 0..1)`.
- Final feeds `ResolveAwarenessStateFromSuspicion` (thresholds unchanged).

## Last reason wins
- `EvidenceConfig.bLastReasonWins` default true. Incoming ReasonTag overwrites record ReasonTag and drives tags/delegates. (No dominant-reason window logic.)

## Location & focus stability
- If incoming has location, Record updates Location/bHasLocation; otherwise keep prior.
- If instigator is valid, focus tag resolves from instigator; if not, focus tag stays as-is (no clearing).

## Delegates/state
- `OnAISuspicionReported` still broadcasts the raw accepted report (pre-blend).
- `OnAIRecordUpdated` and `OnAIAwarenessStateChanged` reflect the blended + evidence-adjusted record.

## Thresholds unaffected
- Awareness thresholds remain in `FSOTS_GSM_AwarenessThresholds`; only FinalSuspicion01 feeding the resolver changed via evidence/blend.
