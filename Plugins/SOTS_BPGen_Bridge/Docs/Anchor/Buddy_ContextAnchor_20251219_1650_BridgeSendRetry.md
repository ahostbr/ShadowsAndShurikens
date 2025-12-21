[CONTEXT_ANCHOR]
ID: 20251219_1650 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeSendRetry | Owner: Buddy
Scope: Improve socket send reliability to avoid malformed JSON responses.

DONE
- Client sockets are forced to blocking mode on acceptance.
- SendResponseAndClose now retries up to 16 sends with short sleeps when Send returns false or zero.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Truncated responses were due to partial sends; blocking sockets + retries should help.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Rebuild SOTS_BPGen_Bridge, restart bridge, rerun bpgen_ping to confirm JSON is valid.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp if needed.
