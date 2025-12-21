[CONTEXT_ANCHOR]
ID: 20251219_194900 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: BuildDependency | Owner: Buddy
Scope: Restore module dependency on SOTS_BlueprintGen while keeping plugin-level cycle removed.

DONE
- Added SOTS_BlueprintGen to PublicDependencyModuleNames in SOTS_BPGen_Bridge.Build.cs.
- Kept SOTS_BlueprintGen plugin reference removed from SOTS_BPGen_Bridge.uplugin.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Editor build should proceed without circular plugin error; compile should find BPGen headers.

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.Build.cs

NEXT (Ryan)
- Re-run ShadowsAndShurikensEditor build; expect no circular dependency warning and no missing-header errors from bridge.

ROLLBACK
- Remove SOTS_BlueprintGen from the bridge Build.cs dependency list; re-add plugin reference if the cycle was not the root cause.
