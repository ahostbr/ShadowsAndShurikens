# Buddy Worklog — 2025-12-26 12:00:00 — Fix UE5.7 FunctionOps compile

## Goal
Unblock UE 5.7 C++ compile errors in SOTS_BPGen_Bridge function-ops by fixing name collisions and UE 5.7 access restrictions.

## What changed
- Avoided accidental resolution of engine/template `MakeError(...)` by renaming the local helper to `MakeOpError(...)` and updating call sites.
- Removed direct access to `UK2Node_FunctionEntry::ExtraFlags` (protected in UE 5.7). Updated the code path to toggle `FUNC_BlueprintPure` via reflected property access (`FindFProperty` + `Get/SetPropertyValue_InContainer`).

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeFunctionOps.cpp

## Notes / Risks / Unknowns
- UNVERIFIED: No full build was executed here; this is based on the compiler diagnostics and UE 5.7 access rules.
- If `ExtraFlags` changes type/name in future engines, the reflection helper may stop applying `BlueprintPure` (current behavior: it will simply return false).

## Verification status
- UNVERIFIED: Awaiting Ryan build/editor compile.

## Follow-ups / Next steps (Ryan)
- Rebuild project in UE 5.7.1; confirm prior C2440 return-conversion and C2248 protected-member errors are gone.
- If any new compile errors appear, paste the new log tail and we’ll triage the next blocker.
