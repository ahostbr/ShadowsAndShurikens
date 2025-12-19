[CONTEXT_ANCHOR]
ID: 20251220_1530 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: ShutdownWeakAsync | Owner: Buddy
Scope: Guard async shutdown/emergency_stop callbacks against dangling self refs.

DONE
- Async handlers for shutdown/emergency_stop now capture AsWeak and Pin() before calling Stop().
- Deleted plugin Binaries and Intermediate after the change.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Crash on Stop is mitigated by removing strong captures in these handlers.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Binaries
- Plugins/SOTS_BPGen_Bridge/Intermediate

NEXT (Ryan)
- Rebuild the plugin and run start/stop plus editor exit to confirm crash is gone.
- If crash persists, inspect remaining async captures or double Stop invocations.

ROLLBACK
- Revert Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp to prior revision and restore binaries from source control if needed.
