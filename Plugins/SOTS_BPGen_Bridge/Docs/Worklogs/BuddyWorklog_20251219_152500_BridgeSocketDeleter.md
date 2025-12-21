# Buddy Worklog — 2025-12-19 15:25:00

## Goal
Fix crash in BPGen Bridge connection handling (EXCEPTION_ACCESS_VIOLATION from shared pointer release destroying sockets improperly).

## What changed
- Wrapped accepted client sockets in a shared pointer with a custom deleter that calls `Close()` and `DestroySocket()` via the socket subsystem instead of the default `delete`.
- Updated `SendResponseAndClose` to only close the socket; destruction is now handled by the shared-pointer deleter to prevent double-destroy.
- Attempted to delete plugin `Binaries/` and `Intermediate/`; delete failed due to locked files (likely editor/UBT holding DLL/PDB).

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Binaries (delete attempted, failed: access denied)
- Plugins/SOTS_BPGen_Bridge/Intermediate (delete attempted, failed: access denied)

## Notes / risks / unknowns
- Access denied on Binaries/Intermediate suggests editor or another process is locking the DLL/PDB. Requires retry after process exit.
- Behavior relies on shared-pointer deleter for cleanup; ensure no other code assumes `SendResponseAndClose` destroys sockets.

## Verification
- Not run (no build/editor rerun yet).

## Follow-ups / next steps
- Close any editor/locking processes, retry deleting Binaries/Intermediate, then rebuild SOTS_BPGen_Bridge.
- Re-run the bridge start/requests to confirm crash is resolved.
