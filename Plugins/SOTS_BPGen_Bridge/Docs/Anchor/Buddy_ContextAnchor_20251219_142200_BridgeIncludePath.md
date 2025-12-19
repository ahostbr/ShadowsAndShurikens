[CONTEXT_ANCHOR]
ID: 20251219_142200 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: IPv4_Header_Path | Owner: Buddy
Scope: Fix missing IPv4Address include path for UE5.7 build.

DONE
- Switched bridge server include to Interfaces/IPv4/IPv4Address.h so FIPv4Address resolves during compile.

VERIFIED
- None (no build/run executed).

UNVERIFIED / ASSUMPTIONS
- Only header path changed; no behavioral impact.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

NEXT (Ryan)
- Rebuild ShadowsAndShurikensEditor to confirm SOTS_BPGen_Bridge compiles.

ROLLBACK
- Revert the include line in SOTS_BPGenBridgeServer.cpp.
