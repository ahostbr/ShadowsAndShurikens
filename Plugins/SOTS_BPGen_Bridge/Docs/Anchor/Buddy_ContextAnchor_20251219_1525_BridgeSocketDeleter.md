[CONTEXT_ANCHOR]
ID: 20251219_1525 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeSocketDeleter | Owner: Buddy
Scope: Prevent socket destruction crash in HandleConnection.

DONE
- Wrapped accepted sockets in a shared pointer with custom deleter that closes and destroys via socket subsystem.
- Updated SendResponseAndClose to only close; destruction handled by deleter.
- Attempted to delete Binaries/Intermediate (failed: files locked).

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Crash was caused by default shared-pointer delete on sockets and double-destroy; custom deleter should fix.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Binaries (delete attempted, access denied)
- Plugins/SOTS_BPGen_Bridge/Intermediate (delete attempted, access denied)

NEXT (Ryan)
- Close any processes holding SOTS_BPGen_Bridge DLL/PDB; delete Binaries and Intermediate; rebuild.
- Exercise bridge start and send requests to confirm no crash.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp and restore Binaries/Intermediate from source control if needed.
