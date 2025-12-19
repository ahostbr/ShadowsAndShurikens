[CONTEXT_ANCHOR]
ID: 20251219_1930 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenCompileHotfix | Owner: Buddy
Scope: Clear immediate BPGen compile blockers from 12/19 UBT log.

DONE
- Fixed EdGraph schema include path so BlueprintGraph headers resolve in BPGen discovery.
- Removed stray #endif in BPGen debug helpers to restore balanced preprocessor guards.
- Implemented AddNodeToMap helper used during graph apply to eliminate undefined static function errors.

VERIFIED
- None (not built or run).

UNVERIFIED / ASSUMPTIONS
- Expect compilation to progress past prior C1083/C1020/C2129 errors; additional issues may appear after rerun.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDiscovery.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDebug.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

NEXT (Ryan)
- Re-run UBT for editor target; confirm BPGen module now builds.
- If new errors surface, capture log for follow-up; clean BPGen Binaries/Intermediate if they reappear.

ROLLBACK
- Revert the touched source files to previous revision.
