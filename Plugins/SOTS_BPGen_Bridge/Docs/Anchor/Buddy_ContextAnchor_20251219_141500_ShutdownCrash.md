[CONTEXT_ANCHOR]
ID: 20251219_141500 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: Shutdown_Crash | Owner: Buddy
Scope: Mitigate engine-exit crash from bridge Stop/Async future teardown.

DONE
- In Stop(), clear AcceptTask after Wait to drop future shared state during shutdown.
- Tried removing Binaries/Intermediate for SOTS_BPGen_Bridge (paths may already be absent).

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Clearing future prevents exit-time crash; needs editor exit test.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Binaries (attempted delete)
- Plugins/SOTS_BPGen_Bridge/Intermediate (attempted delete)

NEXT (Ryan)
- Rebuild, start/stop bridge in Control Center, then exit editor to check for crash regression.

ROLLBACK
- Revert Stop() change and regenerate build outputs.
