[CONTEXT_ANCHOR]
ID: 20251219_1600 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenBuildFlag | Owner: Buddy
Scope: Build.cs fix for BPGen after UBT ModuleType error.

DONE
- Replaced invalid `ModuleRules.ModuleType.Editor` usage with `bIsEditorOnly = true` in Build.cs.
- Attempted Binaries/Intermediate cleanup (paths already absent).

VERIFIED
- None (no build).

UNVERIFIED / ASSUMPTIONS
- Expect Build.cs to parse; no further changes.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs

NEXT (Ryan)
- Rerun UBT; if errors persist, share log (particularly SocketNotifyCopy rules issue).

ROLLBACK
- Revert Build.cs change and restore Binaries/Intermediate if needed.
