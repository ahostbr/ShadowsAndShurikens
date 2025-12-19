[CONTEXT_ANCHOR]
ID: 20251219_140200 | Plugin: SOTS_BlueprintGen | Pass/Topic: SpawnerRegistry_API | Owner: Buddy
Scope: Export spawner registry symbols so SOTS_BPGen_Bridge can link on UE5.7.

DONE
- Added SOTS_BLUEPRINTGEN_API to FSOTS_BPGenResolvedSpawner and FSOTS_BPGenSpawnerRegistry declarations.
- Removed Binaries/Intermediate for SOTS_BlueprintGen and SOTS_BPGen_Bridge post-change.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Exporting symbols fixes bridge linker errors; runtime unaffected.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenSpawnerRegistry.h
- Plugins/SOTS_BlueprintGen/Binaries (deleted)
- Plugins/SOTS_BlueprintGen/Intermediate (deleted)
- Plugins/SOTS_BPGen_Bridge/Binaries (deleted)
- Plugins/SOTS_BPGen_Bridge/Intermediate (deleted)

NEXT (Ryan)
- Rebuild ShadowsAndShurikensEditor to confirm linker errors are resolved.

ROLLBACK
- Revert SOTS_BPGenSpawnerRegistry.h and regenerate Binaries/Intermediate via build.
