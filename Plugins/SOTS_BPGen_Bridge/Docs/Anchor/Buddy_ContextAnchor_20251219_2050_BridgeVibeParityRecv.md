[CONTEXT_ANCHOR]
ID: 20251219_2050 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeVibeParityRecv | Owner: Buddy
Scope: Multi-message buffered receive loop and logging aligned with VibeUE MCP runnable.

DONE
- HandleConnection now buffers UTF-8 chunks, splits on `\n`, processes all complete messages per connection, and keeps the tail for the next recv; per-message audit/dispatch runs via a lambda.
- Added VeryVerbose state/pending-data logs, hex sample logging, and small sleeps to avoid tight loops; MaxRequestBytes guard on buffered data and per-request.
- Retains socket options (NoDelay, 64KB buffers, blocking) set on accept.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Multi-message handling should match VibeUE MCP runnable behavior; assumes one newline-delimited request per message.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Rebuild SOTS_BPGen_Bridge, restart bridge, run bpgen_ping with Verbose logs enabled to confirm framing/output parity.
- If discrepancies remain, check linger/keepalive/backoff against VibeUE Bridge.cpp and adjust accordingly.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp; restore plugin binaries/intermediate if needed.
