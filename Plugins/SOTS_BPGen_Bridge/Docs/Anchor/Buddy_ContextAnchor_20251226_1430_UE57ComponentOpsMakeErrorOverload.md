Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1430 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UE57_ComponentOps_MakeErrorOverload | Owner: Buddy
Scope: Fix ComponentOps MakeError overload resolution for UE 5.7 build

DONE
- Added `MakeError(const TCHAR*, const TCHAR*)` and mixed overloads so `TEXT("...")` calls bind to the local helper instead of UE's `TValueOrError` `MakeError`.

VERIFIED
- 

UNVERIFIED / ASSUMPTIONS
- Assumption: UE build errors `C2440` were caused by overload resolution selecting the `TValueOrError` helper.
- UNVERIFIED: Requires UE build to confirm the file compiles.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeComponentOps.cpp
- Plugins/SOTS_BPGen_Bridge/Docs/Worklogs/BuddyWorklog_20251226_143000_UE57ComponentOpsMakeErrorOverload.md

NEXT (Ryan)
- Run a full editor build; confirm `SOTS_BPGenBridgeComponentOps.cpp` no longer emits return-type conversion errors.

ROLLBACK
- Revert the overload additions in `SOTS_BPGenBridgeComponentOps.cpp`.
