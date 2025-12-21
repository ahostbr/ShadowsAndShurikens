# Buddy Worklog — 2025-12-19 16:00:00

## Goal
Fix malformed TCP responses (bpgen_ping JSON parse errors) caused by partial socket sends.

## What changed
- Updated SendResponseAndClose to loop-send until the full buffer is transmitted (up to 8 iterations) instead of a single send that could truncate JSON. Still closes the socket afterward; destruction handled by the shared-pointer deleter.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

## Notes / risks / unknowns
- No retry/backoff beyond 8 loops; should be sufficient for small JSON responses. If sockets are non-blocking and return 0 repeatedly, responses may still fail.
- No build/editor run yet after this change.

## Verification
- Not run.

## Follow-ups / next steps
- Rebuild SOTS_BPGen_Bridge, restart bridge, and retry `bpgen_ping` to confirm JSON is well-formed.
