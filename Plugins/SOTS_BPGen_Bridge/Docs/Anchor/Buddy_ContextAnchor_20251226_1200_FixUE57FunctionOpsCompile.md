Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1200 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UE57_FunctionOpsCompile | Owner: Buddy
Scope: Fix UE 5.7 compile blockers in BPGen bridge function ops

DONE
- Renamed local helper `MakeError` → `MakeOpError` in SOTS_BPGenBridgeFunctionOps.cpp to avoid colliding with engine/template `MakeError`.
- Updated BlueprintPure handling to avoid direct access of `UK2Node_FunctionEntry::ExtraFlags` (protected in UE 5.7) by using reflected property access.

VERIFIED
- 

UNVERIFIED / ASSUMPTIONS
- Assumes UE 5.7 still exposes `ExtraFlags` as a reflected property named `ExtraFlags` on `UK2Node_FunctionEntry`.
- No build executed after change.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.cpp

NEXT (Ryan)
- Build in UE 5.7.1; confirm C2440 (return conversion) + C2248 (ExtraFlags protected) are resolved.

ROLLBACK
- Revert Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.cpp
