# Buddy Worklog — SOTS_BlueprintGen

## goal
Resolve linker errors in SOTS_BPGen_Bridge by exporting spawner registry symbols from SOTS_BlueprintGen.

## what changed
- Marked FSOTS_BPGenResolvedSpawner and FSOTS_BPGenSpawnerRegistry with SOTS_BLUEPRINTGEN_API so their symbols export for bridge consumers.
- Deleted Binaries/Intermediate for SOTS_BlueprintGen and SOTS_BPGen_Bridge after the change.

## files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenSpawnerRegistry.h
- (Deleted folders) Plugins/SOTS_BlueprintGen/Binaries, Plugins/SOTS_BlueprintGen/Intermediate, Plugins/SOTS_BPGen_Bridge/Binaries, Plugins/SOTS_BPGen_Bridge/Intermediate

## notes + risks/unknowns
- Only export modifiers added; behavior unchanged.

## verification status
- Not built or run; linker fix is UNVERIFIED.

## follow-ups / next steps
- Rebuild ShadowsAndShurikensEditor to verify linker errors are resolved.
