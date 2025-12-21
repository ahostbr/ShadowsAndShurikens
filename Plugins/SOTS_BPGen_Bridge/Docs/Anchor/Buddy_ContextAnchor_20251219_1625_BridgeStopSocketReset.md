[CONTEXT_ANCHOR]
ID: 20251219_1625 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeStopSocketReset | Owner: Buddy
Scope: Fix shutdown crash from shared pointer/socket cleanup in Stop.

DONE
- Listen socket now uses custom deleter (Close + DestroySocket via subsystem) on creation.
- Stop now closes, moves, and resets the listen socket once after waiting for accept task; removed redundant destroy that fought the deleter.
- Retried deleting Binaries/Intermediate (best-effort; may still be locked).

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Crash was due to double-destroy/shared pointer corruption; custom deleter + single reset should prevent it.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Binaries (delete attempted)
- Plugins/SOTS_BPGen_Bridge/Intermediate (delete attempted)

NEXT (Ryan)
- Ensure editor/UBT closed; delete Binaries/Intermediate; rebuild and retest shutdown.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp and restore plugin binaries/intermediate if needed.
