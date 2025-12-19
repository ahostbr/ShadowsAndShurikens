# Buddy Worklog — SOTS_BlueprintGen

## goal
Surface bridge start failures in the BPGen Control Center and avoid no-op clicks when the server pointer is missing.

## what changed
- OnStartBridge now guards against a missing server, logs start success/failure with running/bind info and last start error, and refreshes status.
- OnStopBridge now reacquires the server if needed and logs when the server is unavailable.
- Removed SOTS_BlueprintGen Binaries and Intermediate per plugin edit rules.
- Stub FSOTS_BPGenBridgeServer::Start now returns bool (signature aligned with bridge server) to fix Control Center compile.

## files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.cpp
- Plugins/SOTS_BlueprintGen/Binaries (deleted)
- Plugins/SOTS_BlueprintGen/Intermediate (deleted)

## notes + risks/unknowns
- Start may still fail due to bind/config; now reported in UI log/status.

## verification status
- Not built/run; UNVERIFIED.

## follow-ups / next steps
- Rebuild, open Control Center, click Start/Stop, and confirm log shows running state or explicit error.
