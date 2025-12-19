# Buddy Worklog — 2025-12-14 20:55:20 — AIPercSuspicion

## Goal
Stabilize AIPerception suspicion/detection math with centralized tuning, hysteresis, and GSM-safe reporting while preserving current feel.

## Changes
- Routed per-frame suspicion math through `UpdateSuspicionModel` with per-sense strengths, clamping, and hysteresis defaults.
- Normalized sight/shadow stimuli (visibility multiplier, one-shot hearing) and ensured GSM reporting uses the consolidated output and reason tags.
- Kept tag transitions but now keyed off sanitized suspicion values.

## Files
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp

## Verification
- Not run (per instructions).

## Cleanup
- Deleted SOTS_AIPerception `Binaries/` and `Intermediate/` after edits.

## Follow-ups
- None; ready for build/PIE verification when allowed.
