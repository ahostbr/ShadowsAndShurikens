[CONTEXT_ANCHOR]
ID: 20251219_2110 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeVibeParityBackoff | Owner: Buddy
Scope: Match VibeUE accept/recv backoff and transient error handling.

DONE
- Accept loop now waits 0.1s when no pending connections and after failed Accept.
- Recv loop treats SE_EWOULDBLOCK/SE_EINTR as transient: Verbose log, 10ms sleep, then continue; other errors still break.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Backoff and transient handling should reduce spin and match VibeUE MCP runnable pacing.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Rebuild SOTS_BPGen_Bridge, restart bridge, run bpgen_ping with Verbose logging to confirm framing/output parity.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp; restore plugin binaries/intermediate if needed.
