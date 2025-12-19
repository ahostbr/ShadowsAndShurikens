[CONTEXT_ANCHOR]
ID: 20251219_120000 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: SPINE_I_ReplayRegression | Owner: Buddy
Scope: Bridge protocol flags + graph edit endpoints

DONE
- Added protocol_version + features in responses; protocol mismatch returns ERR_PROTOCOL_MISMATCH
- Added bridge routes for delete_node/delete_link/replace_node; serialize error_code

VERIFIED
- None (code-only)

UNVERIFIED / ASSUMPTIONS
- New actions execute correctly on live assets

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGenBridgeServer.h
- Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md

NEXT (Ryan)
- Run replay harness; confirm protocol mismatch handling and new actions via bridge
- Update expected reports

ROLLBACK
- Revert bridge server cpp/h and protocol doc changes
