# SOTS GSM — Behavior Rollup (Prompt 6)

## 1) Ownership model (GSM vs AIPerception)
- AIPerception owns stimuli, sensing, and raw perception events; GSM only ingests normalized suspicion via ReportAISuspicionEx.
- GSM owns AIRecord state, awareness tier resolution, evidence stacking, last-reason wins, tag application, and the global alertness meter/decay.
- GSM does not perform per-AI decay; per-AI suspicion only changes on explicit reports.

## 2) Inputs (FSOTS_AISuspicionReport)
- SubjectAI: weak AActor of the reporting AI (required).
- Suspicion01: normalized suspicion 0..1 (clamped on ingest).
- ReasonTag: optional; when valid, replaces record reason (last reason wins).
- Location/bHasLocation: optional world position for the event.
- InstigatorActor: optional source actor; feeds focus tag resolution.
- TimestampSeconds: optional world time; GSM fills when absent.

## 3) Per-AI outputs
- AIRecord fields: SubjectAI, Suspicion01 (FinalSuspicion01), AwarenessState, AwarenessTierTag, ReasonTag, FocusTag, RecentEvents, bHasLocation/Location, InstigatorActor, TimestampSeconds.
- Awareness mapping (defaults): Calm <0.25, Investigating >=0.25, Searching >=0.50, Alerted >=0.80, Hostile >=0.95 (see FSOTS_GSM_AwarenessThresholds).
- Last reason wins: incoming ReasonTag overwrites the record and drives tags/delegates when valid.
- Focus target: InstigatorActor resolves to Player/Unknown; if none provided, record keeps prior focus (or defaults to Unknown).

## 4) Evidence stacking
- Rolling window: RecentEvents pruned to WindowSeconds and MaxEventsPerAI (oldest dropped first).
- Points: EvidencePointsPerEvent * optional reason multiplier * SameInstigatorMultiplier when the instigator repeats; ignore events below MinSuspicionToCountAsEvidence.
- Bonus: (Points / MaxEvidencePoints) * MaxEvidenceSuspicionBonus, clamped 0..MaxEvidenceSuspicionBonus.
- Blend: FinalSuspicion01 = clamp(max(Incoming, Lerp(Incoming, Prev, PriorSuspicionBlendAlpha)) + EvidenceBonus, 0..1).
- FinalSuspicion01 feeds awareness resolution, delegates, tags, and global alertness inputs.

## 5) Global alertness
- Increase per accepted report: (Suspicion01 * SuspicionWeight) + TierWeight(AwarenessState) + EvidenceBonus * EvidenceBonusWeight, rate-limited by MaxIncreasePerSecond.
- Decay: timer-driven toward Baseline at DecayPerSecond; clamped to [MinValue, MaxValue].
- Delegate: OnGlobalAlertnessChanged broadcasts when |delta| >= UpdateMinDelta or band tag changes.
- Band tags (player): SAS.Global.Alertness.{Calm,Tense,Alert,Critical}; applied/removed on band changes.

## 6) Tags written by GSM
- AI per-actor: SAS.AI.Alert.{Calm,Investigating,Searching,Alerted,Hostile}; SAS.AI.Reason.{Sight,Hearing,Shadow,Damage,Generic}; SAS.AI.Focus.{Player,Unknown}.
- Player/global: SOTS.Stealth.State.{Hidden,Cautious,Danger,Compromised}; SOTS.Stealth.Light.{Bright,Dark}; SAS.Global.Alertness.{Calm,Tense,Alert,Critical}.
- Missing-tag fallback: RequestTagSafe (ErrorIfNotFound=false); logs once per missing tag name, skips apply when invalid.

## 7) Smoke tests (executed via python simulation; PASS)
- Per-AI state mapping (0.1,0.25,0.45,0.75,0.95) -> Calm, Investigating, Investigating, Searching, Hostile (PASS).
- Evidence stacking window: three 0.3 reports yielded Points=3.5, Bonus≈0.146, Final≈0.446; after prune to one event Points=1.0, Bonus≈0.042, Final≈0.342 (PASS).
- Last reason wins: Hearing -> Sight -> Damage produced CurrentReasonTag=Damage (PASS).
- Global alertness: three 0.75 reports (Searching state) rose to ≈0.2625; 2s decay with DecayPerSecond=0.05 drifted to ≈0.1625 (PASS). UpdateMinDelta respected via monotonic increase >0.001 (PASS).
- Tags update: tier change Hidden->Cautious flagged as swap; missing tag simulated as invalid with single warning gate (PASS).

Notes: Tests are deterministic formula checks mirroring GSM logic (no UE runtime). All results align with configured defaults and prompt contracts.
