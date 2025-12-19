# Buddy Worklog — 2025-12-15 23:16:50 — AIPerception→GSM 1C

## Goal
Implement target 1C: GSM accepts context-rich AI suspicion reports (reason, location, instigator) without breaking legacy callers; update AIPerception to use new API; add contract doc.

## What I changed
- Added `FSOTS_AISuspicionReport`, delegate, storage map, and getter to GSM; new `ReportAISuspicionEx` while keeping legacy wrapper.
- GSM now stores last report per AI, broadcasts `OnAISuspicionReported` on accepted ingest, and retains existing ingestion/throttle behavior.
- AIPerception now calls `ReportAISuspicionEx` with reason tag, optional location, and primary target as instigator when available.
- Documented the contract in `Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_AISuspicionReporting_Contract_20251215_2316.md`.

## Files touched
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp
- Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp
- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_AISuspicionReporting_Contract_20251215_2316.md
- Plugins/Docs/BuddyWorklog_20251215_231650_AIPercGSM1C.md (this log)

## Verification
- Not built (per instructions); logic follows existing ingest/throttle path, legacy API remains.
- No binaries/intermediate cleanup performed yet.
