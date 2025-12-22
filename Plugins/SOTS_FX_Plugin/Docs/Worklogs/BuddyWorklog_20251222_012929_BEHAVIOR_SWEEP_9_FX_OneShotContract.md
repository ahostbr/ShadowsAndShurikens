# Buddy Worklog - SOTS_FX_Plugin One-Shot Contract Sweep (20251222_012929)

Goal
- Lock FXManager one-shot entrypoint behavior (overflow reject+log, no UI dependency) and check caller compliance.

Changes
- Added overflow log throttling via `OverflowLogCooldownSeconds` to keep reject warnings non-spammy.
- Confirmed one-shot entrypoints remain on FXManager subsystem with no UI dependencies.
- Scanned suite plugins for direct SpawnEmitter/SpawnSound/SpawnSystem calls; only non-SOTS plugin hit noted.

Notes
- Lawfile path `/mnt/data/SOTS_Suite_ExtendedMemory_LAWs.txt` not found locally; used repo-root `SOTS_Suite_ExtendedMemory_LAWs.txt`.
- No build/run executed.

Files touched
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp
- Plugins/SOTS_FX_Plugin/Docs/SOTS_FX_OneShotContract.md

Cleanup
- Deleted Plugins/SOTS_FX_Plugin/Binaries and Plugins/SOTS_FX_Plugin/Intermediate.
