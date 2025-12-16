# SOTS GSM — Global Alertness Contract (Prompt 4)

**Definition**
- GlobalAlertness is a mission-wide tension/heat meter in [0..1]. It rises when AI reports suspicion and decays toward a baseline when idle. Per-AI suspicion remains event-driven (no per-AI decay here).

**Config (FSOTS_GSM_GlobalAlertnessConfig)**
- Bounds: `Baseline` defaults 0.0, clamped to `[MinValue, MaxValue]` (defaults 0..1).
- Decay: `DecayPerSecond` units per second toward `Baseline` (timer-driven).
- Increase weights: `SuspicionWeight` applied to incoming Suspicion01; tier weights: `Weight_Unaware`, `Weight_Suspicious`, `Weight_Investigating`, `Weight_Alert`, `Weight_Combat` mapped from Calm/Investigating/Searching/Alerted/Hostile.
- Evidence: optional `EvidenceBonusWeight` multiplied by the Prompt 3 evidence bonus when available.
- Smoothing: optional clamp `MaxIncreasePerSecond`; broadcast threshold `UpdateMinDelta` to avoid spam.
- Timer cadence: decay/update tick runs at 0.25s.

**Runtime State**
- `GlobalAlertness` starts at clamped `Baseline` on reset/init and is clamped to `[MinValue, MaxValue]` on config changes.
- `LastGlobalAlertnessUpdateTimeSeconds` tracks the last applied change/decay time; decay uses this for `dt`.

**Decay Path**
- Repeating timer (0.25s) calls `UpdateGlobalAlertnessDecay`:
  - `dt = Now - LastGlobalAlertnessUpdateTimeSeconds`
  - Move toward clamped `Baseline` by `DecayPerSecond * dt`, clamp to bounds.
  - Broadcast/tag update if change ≥ `UpdateMinDelta` (or band tag changed).

**Increase Path (ReportAISuspicionEx)**
- On every accepted AI suspicion report, compute:
  - `Increase = (Suspicion01 * SuspicionWeight) + TierWeight(NewState)`
  - If evidence bonus > 0: `Increase += EvidenceBonus * EvidenceBonusWeight`
  - Clamp rate if `MaxIncreasePerSecond > 0`: `Increase = min(Increase, MaxIncreasePerSecond * dt)` where `dt` is time since last global update (fallback: timer interval).
- Apply: `GlobalAlertness = clamp(GlobalAlertness + Increase, MinValue, MaxValue)`; update timestamp.

**Delegate**
- `FSOTS_OnGlobalAlertnessChanged(NewValue, OldValue)` broadcasts when |delta| ≥ `UpdateMinDelta` or band tag changed.

**Global Tags (banded)**
- 0.00–0.25: `SAS.Global.Alertness.Calm`
- 0.25–0.50: `SAS.Global.Alertness.Tense`
- 0.50–0.75: `SAS.Global.Alertness.Alert`
- 0.75–1.00: `SAS.Global.Alertness.Critical`
- Prior band tag is removed before the new one is applied; tags attach to the player actor when available.

**Notes**
- No per-AI suspicion decay added; per-AI suspicion remains event-driven via ReportAISuspicionEx and Prompt 3 history/evidence logic.
- Global alertness responds to config changes by clamping to new bounds; timer is restarted on resets.
