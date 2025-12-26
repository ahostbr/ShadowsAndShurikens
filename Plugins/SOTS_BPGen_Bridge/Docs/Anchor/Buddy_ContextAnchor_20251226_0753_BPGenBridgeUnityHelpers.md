Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_0753 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UnityHelperDedup | Owner: Buddy
Scope: Deduplicate private helper functions to avoid unity-build ODR collisions.

DONE
- Added `SOTS_BPGenBridgePrivateHelpers.h` with shared `inline` helpers in `SOTS_BPGenBridgePrivate`.
- Updated bridge ops `.cpp` files to include the shared header and call helpers via `SOTS_BPGenBridgePrivate::...`.
- Renamed VariableOps local error helper to `MakeVariableOpError` and updated all call sites.

VERIFIED
- None (no build/editor verification in this session).

UNVERIFIED / ASSUMPTIONS
- Assumes prior unity-build redefinition errors are resolved once rebuilt.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgePrivateHelpers.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeAssetOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeBlueprintOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeComponentOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeVariableOps.cpp

NEXT (Ryan)
- Build editor target with unity builds enabled.
- Confirm BPGen Bridge no longer emits helper redefinition/ODR errors.

ROLLBACK
- Revert the files listed above.
