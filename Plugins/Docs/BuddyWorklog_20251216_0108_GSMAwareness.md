# Buddy Worklog — 2025-12-16 01:08 UTC — GSM Awareness

## Goal
Implement GSM awareness spine wiring for `ReportAISuspicionEx` (state enum, thresholds, per-AI records, delegates, tag publishing) without adding features beyond the prompt.

### Prompt 3 follow-up
- Add bounded per-AI suspicion event history and evidence stacking with numeric blending.

## What changed
- Added AI awareness enum/thresholds/records to GSM types and config.
- Updated GSM subsystem to track per-AI records, resolve awareness tiers, publish tags, and broadcast awareness delegates while keeping legacy suspicion ingest intact.
- Registered AI focus gameplay tags and documented AI reason/focus branches in the locked schema.
- Captured the awareness contract in a GSM doc.
- Implemented bounded per-AI suspicion history and evidence stacking (blend + bonus) feeding FinalSuspicion01 while keeping raw report broadcast.
- Added evidence/history contract doc.

## Files touched
- Config/DefaultGameplayTags.ini
- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_AIAwarenessSpine_20251216_0105.md
- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_EvidenceStacking_History_Contract_20251216_0135.md
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthTypes.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp
- Plugins/SOTS_TagManager/Docs/SAS_TagSchema_Locked.md

## Verification
- Not run (code changes only; no build per instructions).

## Cleanup
- Deleted Plugins/SOTS_GlobalStealthManager/Binaries and Intermediate.

## Follow-ups
- Consider extending focus detection (e.g., dragon) once target tagging is defined.
