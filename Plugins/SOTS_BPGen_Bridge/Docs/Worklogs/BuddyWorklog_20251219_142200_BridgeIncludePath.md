# Buddy Worklog — SOTS_BPGen_Bridge

## goal
Fix UE5.7 build failure in SOTS_BPGen_Bridge due to missing IPv4Address header include path.

## what changed
- Updated bridge server include to use UE5.7 path `Interfaces/IPv4/IPv4Address.h` so FIPv4Address resolves.

## files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

## notes + risks/unknowns
- No functional changes; header path only.
- Build still needs to be rerun.

## verification status
- Not built or run; change is UNVERIFIED.

## follow-ups / next steps
- Rebuild ShadowsAndShurikensEditor to confirm bridge now compiles.
