# Buddy Worklog — SOTS_BPGen_Bridge

## goal
Harden shutdown/emergency stop paths so async callbacks do not dereference a destroyed server instance.

## what changed
- Switched async handlers for `emergency_stop` and `shutdown` actions to capture a weak pointer and guard Stop() with Pin().
- Removed plugin Binaries and Intermediate after the change per SOTS law.

## files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Binaries (deleted)
- Plugins/SOTS_BPGen_Bridge/Intermediate (deleted)

## notes + risks/unknowns
- Shutdown crash path still unverified; other strong captures may remain elsewhere.

## verification status
- Not built/run; UNVERIFIED.

## follow-ups / next steps
- Rebuild and exercise start/stop plus editor exit to see if crash persists.
- If crashes remain, continue sweeping for remaining strong captures or double Stop paths.
