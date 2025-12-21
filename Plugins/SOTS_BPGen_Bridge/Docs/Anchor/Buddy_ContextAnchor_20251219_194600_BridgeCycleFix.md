[CONTEXT_ANCHOR]
ID: 20251219_194600 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: CycleRemoval | Owner: Buddy
Scope: Break circular dependency with SOTS_BlueprintGen.

DONE
- Removed SOTS_BlueprintGen from bridge module dependencies in SOTS_BPGen_Bridge.Build.cs.
- Removed SOTS_BlueprintGen plugin reference from SOTS_BPGen_Bridge.uplugin.

VERIFIED
- None (no build/run performed).

UNVERIFIED / ASSUMPTIONS
- Bridge does not require BlueprintGen symbols at link/runtime; expect build to succeed once cycle is broken.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.Build.cs
- Plugins/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.uplugin

NEXT (Ryan)
- Rebuild ShadowsAndShurikensEditor; confirm cycle warning gone and bridge module compiles.
- If unresolved external errors appear, add specific dependencies (not BlueprintGen) or stub missing symbols.

ROLLBACK
- Re-add SOTS_BlueprintGen to the bridge PublicDependency list and uplugin Plugins array.
