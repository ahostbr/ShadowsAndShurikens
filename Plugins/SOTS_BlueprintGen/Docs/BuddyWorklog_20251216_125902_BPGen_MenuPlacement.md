# Buddy Worklog — 2025-12-16 12:59:02 — BPGen menu placement

## Goal
Move SOTS BPGen Runner menu entry under the SOTS Tools section (away from BEP Tools).

## What changed
- Updated SOTS_BlueprintGen menu registration to use the `SOTS_Tools` section in the LevelEditor Window menu so the BPGen Runner shows under SOTS Tools.

## Files touched
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BlueprintGen.cpp

## Verification
- UNVERIFIED (menu wiring only; no build run).

## Cleanup
- Deleted Plugins/SOTS_BlueprintGen/Binaries and Plugins/SOTS_BlueprintGen/Intermediate per policy.
