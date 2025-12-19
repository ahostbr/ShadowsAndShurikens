[CONTEXT_ANCHOR]
ID: 20251219_183900 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenCompileUE57 | Owner: Buddy
Scope: Clear UE5.7 UBT compile errors for BPGen ensure/graph resolver.

DONE
- Swapped GraphResolver include to EdGraphSchema_K2.h path.
- ImportText_Direct now uses a non-const owner; FunctionEntry pure/const flags set via reflected ExtraFlags property (adds warning if missing).
- Widget binding creation writes ObjectName as FString to align with updated delegate binding API.
- Removed SOTS_BlueprintGen Binaries/Intermediate after edits.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- ExtraFlags property still exists on UK2Node_FunctionEntry; if removed, pure/const flags will stay default.
- Assume widget binding ObjectName type change matches UE5.7 layout.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenGraphResolver.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenEnsure.cpp

NEXT (Ryan)
- Re-run UBT for ShadowsAndShurikensEditor; confirm BPGen builds clean.
- Verify EnsureFunction still produces pure/const functions and widget bindings create/update correctly.
- Watch for ExtraFlags warning in logs; if present, share for follow-up.

ROLLBACK
- Revert Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenGraphResolver.cpp and SOTS_BPGenEnsure.cpp; regenerate binaries via next build.
