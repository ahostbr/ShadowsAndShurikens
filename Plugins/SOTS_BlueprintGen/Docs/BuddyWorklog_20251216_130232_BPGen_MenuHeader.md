# Buddy Worklog — 2025-12-16 13:02:32 — BPGen menu header fix

## Goal
Restore SOTS Tools section so BPGen Runner appears under SOTS Tools instead of BEP Tools.

## What changed
- Window menu registration now explicitly creates the `SOTS_Tools` section with the label "SOTS Tools" before adding the BPGen Runner entry, preventing it from falling under BEP Tools.

## Files touched
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BlueprintGen.cpp

## Verification
- UNVERIFIED (menu-only change; no build/run in this pass).

## Cleanup
- Deleted Plugins/SOTS_BlueprintGen/Binaries and Plugins/SOTS_BlueprintGen/Intermediate per policy.
