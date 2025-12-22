# Buddy Worklog - SOTS_INV InvSP Exception Integration (20251222_011313)

Goal
- Enforce InvSP sealed UI handling in SOTS_INV: adapter-only routing, external menu lifecycle notifications, and no direct UI ownership.

Changes
- Reworked inventory UI helpers to route through SOTS_UI router adapter calls instead of provider UI hooks.
- Added external menu lifecycle notifications (opened/closed) alongside adapter-based open/close requests.
- Kept item identity rule intact (ItemId == ItemTag name) and provider determinism unchanged.

Notes
- Lawfile path `/mnt/data/SOTS_Suite_ExtendedMemory_LAWs.txt` not found locally; used repo-root `SOTS_Suite_ExtendedMemory_LAWs.txt` for reference.
- No build/run executed.

Files touched
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp

Cleanup
- Deleted Plugins/SOTS_INV/Binaries and Plugins/SOTS_INV/Intermediate.
