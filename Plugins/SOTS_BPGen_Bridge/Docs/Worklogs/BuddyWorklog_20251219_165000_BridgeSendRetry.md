# Buddy Worklog — 2025-12-19 16:50:00

## Goal
Address malformed JSON responses from bpgen_ping by ensuring full TCP send and blocking sockets.

## What changed
- Accepted client sockets are now explicitly set to blocking mode in HandleConnection.
- SendResponseAndClose now retries up to 16 send attempts with tiny sleeps on zero/failed sends to reduce truncated responses.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

## Notes / risks / unknowns
- Still no build/run after change; behavior depends on socket send semantics. If sends keep failing, responses may remain truncated.

## Verification
- Not run.

## Follow-ups / next steps
- Rebuild SOTS_BPGen_Bridge, restart bridge, retry bpgen_ping to confirm valid JSON.
