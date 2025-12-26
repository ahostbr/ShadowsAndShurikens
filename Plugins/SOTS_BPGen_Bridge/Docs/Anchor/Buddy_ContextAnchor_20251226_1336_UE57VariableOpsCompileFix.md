Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1336 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: UE57_VariableOpsCompileFix | Owner: Buddy
Scope: UE 5.7 compile unblock for BPGen VariableOps

DONE
- Renamed helper `MakeError` → `MakeOpError` in `SOTS_BPGenBridgeVariableOps.cpp` to avoid name collision.
- Updated tooltip/FriendlyName handling to use `FString` (no `.ToString()`, no `FText::FromString`).
- Fixed `JsonObjectOrNull` to avoid conditional operator type mismatch.
- Removed usage of `FSOTS_BPGenEnsureResult.Errors` (struct has `ErrorMessage`/`ErrorCode`/`Warnings`).

VERIFIED
- UNVERIFIED (no build/editor run).

UNVERIFIED / ASSUMPTIONS
- Assumed `FBPVariableDescription::FriendlyName` is `FString` in this UE version (per compile diagnostics).

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeVariableOps.cpp

NEXT (Ryan)
- Build the project in UE 5.7.1; confirm `SOTS_BPGen_Bridge` compiles.

ROLLBACK
- Revert the file above.
