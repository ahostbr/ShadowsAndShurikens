# Buddy Worklog — SOTS_BPGen_Bridge

## goal
Address crash on engine exit in FSOTS_BPGenBridgeServer::Stop due to lingering async future state.

## what changed
- After waiting on the accept-thread future in Stop(), we now clear the future (AcceptTask = TFuture<void>()) to drop shared state during shutdown.
- Attempted to remove Binaries/Intermediate for SOTS_BPGen_Bridge (folders may have already been absent).

## files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- (Tried) Plugins/SOTS_BPGen_Bridge/Binaries, Plugins/SOTS_BPGen_Bridge/Intermediate

## notes + risks/unknowns
- Cleanup commands reported non-zero exit (folders likely already missing); no manual verification of deletion.
- Crash path was during module shutdown; need to re-test editor exit.

## verification status
- Not built or run; fix is UNVERIFIED.

## follow-ups / next steps
- Rebuild and reopen editor; start/stop bridge, then exit editor to confirm crash no longer occurs.
