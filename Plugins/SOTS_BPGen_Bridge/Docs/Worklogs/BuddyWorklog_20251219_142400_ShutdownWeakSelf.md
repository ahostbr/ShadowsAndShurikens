# Buddy Worklog — SOTS_BPGen_Bridge

## goal
Mitigate exit-time crash in FSOTS_BPGenBridgeServer::Stop by removing strong self-capture in AcceptLoop.

## what changed
- AcceptLoop async task now captures a weak pointer (AsWeak) and pins before running, reducing lifetime/reference issues during module shutdown.
- Cleaned bridge Binaries/Intermediate after the change.

## files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- (Deleted folders) Plugins/SOTS_BPGen_Bridge/Binaries, Plugins/SOTS_BPGen_Bridge/Intermediate

## notes + risks/unknowns
- Crash path still unverified; need to retest editor exit.

## verification status
- Not built/run; change UNVERIFIED.

## follow-ups / next steps
- Rebuild, start/stop bridge, exit editor to confirm crash resolved.
