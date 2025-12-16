# Buddy Worklog — 2025-12-16 12:59:02 — BEP menu trim

## Goal
Leave only the BEP exporter and Node JSON panel entries under the BEP Tools menu.

## What changed
- Removed extra BEP menu entries (Export Selection, Copy Selection, Export/Import Comments, Write Golden Samples, Self Check) so only Full Export and Node JSON Panel remain.

## Files touched
- Plugins/BEP/Source/BEP/Private/BEP.cpp

## Verification
- UNVERIFIED (UI/menu change only; no build run).

## Cleanup
- Deleted Plugins/BEP/Binaries and Plugins/BEP/Intermediate per policy.
