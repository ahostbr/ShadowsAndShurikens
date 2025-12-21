# Buddy Worklog — 2025-12-19 20:50:00

## Goal
Closer 1:1 parity with VibeUE MCP runnable: multi-message buffered recv, connection reuse, and richer logging/guards.

## What changed
- HandleConnection now loops over buffered UTF-8 data, splits on `\n`, processes all complete messages per connection, and retains the tail for subsequent reads (mirrors VibeUE multi-message handling).
- Added per-iteration logging for connection state/pending data (VeryVerbose) and recv outcomes plus hex sample logging, with a small sleep to avoid tight loops.
- Added MaxRequestBytes guard on the buffered message and per-request checks; added request-level audit and dispatch inside a per-message lambda so each message gets its own timing/audit.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

## Notes / risks / unknowns
- Behavior unverified (no build/run). Multiple messages per connection are now allowed; assumes one response per newline-terminated request.
- VeryVerbose logging could be noisy if verbosity is raised; current default log level should keep it silent.

## Verification
- Not run.

## Follow-ups / next steps
- Rebuild SOTS_BPGen_Bridge, restart bridge, run bpgen_ping; capture Verbose logs to confirm framing/output parity.
- If mismatches persist, align any remaining socket option/backoff differences (linger/keepalive) after reviewing VibeUE Bridge.cpp.
