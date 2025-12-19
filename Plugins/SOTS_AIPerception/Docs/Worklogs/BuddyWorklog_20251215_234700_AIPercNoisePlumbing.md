# Buddy Worklog — 2025-12-15 23:47:00 — AIPerception Noise Plumbing

## Goal
Plumb noise instigator/tag through AIPerception, add relation-aware policy/cooldown/range handling, and forward context to GSM without breaking BP signatures.

## What I changed
- Added noise stimulus policy struct and ignore flags to guard perception config.
- Propagated instigator + NoiseTag through Library→Subsystem→Component; component now classifies instigator relation, applies policy (range/cooldown), and adds suspicion impulse with GSM ReportAISuspicionEx (reason/location/instigator).
- Added per-tag cooldown map and telemetry for drops/applied deltas.
- Documented the contract in `Plugins/SOTS_AIPerception/Docs/SOTS_AIPerception_NoiseStimuli_Contract_20251215_2346.md`.

## Files touched
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionSubsystem.h
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionSubsystem.cpp
- Plugins/SOTS_AIPerception/Docs/SOTS_AIPerception_NoiseStimuli_Contract_20251215_2346.md
- Plugins/Docs/BuddyWorklog_20251215_234700_AIPercNoisePlumbing.md (this log)

## Verification
- Not built (pending user/build pipeline). No binaries/intermediate present after cleanup.
