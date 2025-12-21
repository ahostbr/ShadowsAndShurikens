[CONTEXT_ANCHOR]
ID: 20251219_1710 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeVibeSocketParity | Owner: Buddy
Scope: Apply VibeUE socket setup patterns to bridge connections.

DONE
- Listen socket now sets reuse-addr and non-blocking after creation.
- Client sockets set NoDelay and 64KB send/recv buffers before switching to blocking mode.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Matching VibeUE socket options should improve stability and reduce truncated responses.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Rebuild SOTS_BPGen_Bridge, restart bridge, retry bpgen_ping to confirm valid JSON.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp if needed.
