[CONTEXT_ANCHOR]
ID: 20251219_1620 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenBuildCleanup | Owner: Buddy
Scope: Resolve Build.cs CS0103 (bIsEditorOnly) and prep for next UBT.

DONE
- Removed invalid `bIsEditorOnly` usage in Build.cs; kept standard PCHUsage.
- Attempted Binaries/Intermediate cleanup (paths absent; no deletions).

VERIFIED
- None (no build).

UNVERIFIED / ASSUMPTIONS
- Build.cs should now parse; no editor-only flag applied. Editor dependencies remain in module lists.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs

NEXT (Ryan)
- Re-run UBT; if errors remain, share the log (especially any SocketNotifyCopy rules issues once BPGen parses).

ROLLBACK
- Revert Build.cs change; restore plugin binaries/intermediate if needed.
