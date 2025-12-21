# Buddy Worklog — 2025-12-19 20:35:00

## Goal
Match VibeUE-style buffered receive/parsing to reduce malformed JSON and add response logging/normalization.

## What changed
- Accept loop now waits on HasPendingConnection with a 50ms backoff before Accept to reduce busy spin.
- Per-connection recv path now buffers UTF-8 chunks into a FString, splits on `\n`, trims `\r`, and processes the first complete message; logs the first 50 bytes in hex for debugging and tracks byte limits before responding.
- SendResponseAndClose normalizes to a single trailing `\n` and logs the full response length/string before the retrying send loop.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

## Notes / risks / unknowns
- Behavior unverified (no build/run). Blocking Recv loop breaks on socket error/close; assumes one request per connection.
- Hex logging and response logging are Verbose; could be noisy if verbosity is raised.

## Verification
- Not run.

## Follow-ups / next steps
- Rebuild SOTS_BPGen_Bridge, restart bridge service, rerun bpgen_ping to confirm JSON correctness.
- If malformed persists, use new response log to capture raw JSON and adjust buffering/tailing logic as needed.
