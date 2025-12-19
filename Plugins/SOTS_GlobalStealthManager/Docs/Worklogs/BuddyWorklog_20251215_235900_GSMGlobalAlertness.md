# Buddy Worklog — GSM Global Alertness (Prompt 4)

## Goal
Add the global alertness metric (Prompt 4/6) to GSM with decay, report-driven increases, delegate/tag outputs, and documentation while leaving per-AI suspicion event-driven.

## Changes
- Added `FSOTS_GSM_GlobalAlertnessConfig` (bounds, decay, weights, evidence influence, broadcast threshold) and exposed it on the GSM scoring config.
- Introduced global alertness runtime state (`GlobalAlertness`, `LastGlobalAlertnessUpdateTimeSeconds`, band tag cache, timer handle) plus Blueprint getter and multicast delegate.
- Implemented timer-driven decay (0.25s cadence) toward a clamped baseline with broadcast/tag updates on meaningful deltas or band changes.
- Applied report-driven increases inside `ReportAISuspicionEx` using suspicion weight, awareness tier weight mapping, optional evidence bonus weight, and rate clamp; clamps to config bounds and updates timestamp.
- Added global alertness band tags (`SAS.Global.Alertness.{Calm|Tense|Alert|Critical}`) applied to the player actor with drift repair.
- Documented the contract in `SOTS_GSM_GlobalAlertness_Contract_20251215_2345.md`.

## Files Touched
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthTypes.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp
- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_GlobalAlertness_Contract_20251215_2345.md

## Verification
- Static analysis: `get_errors` on touched GSM headers/cpp passed (no errors).
- Builds/tests not run per instructions.

## Cleanup
- Deleted Plugins/SOTS_GlobalStealthManager/Binaries and Plugins/SOTS_GlobalStealthManager/Intermediate after edits.

## Follow-ups
- Consider exposing timer interval/config if tuning demands finer control.
- Evaluate default weights once audio/UI consumers hook into the global alertness delegate/tags.
