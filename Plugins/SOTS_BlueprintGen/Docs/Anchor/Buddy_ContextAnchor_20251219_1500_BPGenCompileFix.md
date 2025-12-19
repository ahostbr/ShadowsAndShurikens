[CONTEXT_ANCHOR]
ID: 20251219_1500 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenCompileFix | Owner: Buddy
Scope: Restore BPGen build helpers and includes after UE5.7 compile errors.

DONE
- Added missing K2 node includes and helper forward declarations in SOTS_BPGenBuilder.cpp (FunctionEntry/Result, ResolveGraphForApply, package/name helpers).
- Reinstated EnsureBlueprintVariableFromNodeSpec with logging, pin type building, and variable creation guard; added helper functions FindFunctionGraph, LoadStructFromPath, GetNormalizedPackageName, GetSafeObjectName, ResolvePinCategoryString.
- Fixed format-string args (GetPinDirectionText) and owner resolution for variable spawners; cleaned brace errors in AddPinsFromSpec.
- Removed bad BlueprintEditorUtils include in SOTS_BPGenDebug.cpp.
- Deleted plugin Binaries/Intermediate folders post-edit.

VERIFIED
- None (no build run).

UNVERIFIED / ASSUMPTIONS
- Expect compile errors resolved by added includes/helpers; no compiler confirmation yet.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDebug.cpp

NEXT (Ryan)
- Run UBT for SOTS_BlueprintGen; confirm compile passes or capture new errors.
- If errors persist, review new helpers and remaining log locations for missing APIs or brace issues.

ROLLBACK
- Revert the two touched source files and restore deleted plugin Binaries/Intermediate if needed (from VCS or backups).
