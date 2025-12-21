[CONTEXT_ANCHOR]
ID: 20251219_1600 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BridgeJsonSendLoop | Owner: Buddy
Scope: Fix malformed JSON responses from bridge TCP replies.

DONE
- Added looped send in SendResponseAndClose to ensure entire JSON buffer is transmitted; limited to 8 iterations.

VERIFIED
- None (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Partial sends were causing truncated JSON; looped send should resolve malformed responses.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Rebuild SOTS_BPGen_Bridge, restart bridge, rerun bpgen_ping to confirm JSON parses correctly.

ROLLBACK
- Revert SOTS_BPGenBridgeServer.cpp to prior revision if needed.
