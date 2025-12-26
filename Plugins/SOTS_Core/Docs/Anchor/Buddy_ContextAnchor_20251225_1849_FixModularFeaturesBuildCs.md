Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1849 | Plugin: SOTS_Core | Pass/Topic: FixModularFeaturesBuildCs | Owner: Buddy
Scope: Remove invalid Build.cs module dependency blocking UBT.

DONE
- Removed `"ModularFeatures"` from `PrivateDependencyModuleNames` in `Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs`.
- Deleted `Plugins/SOTS_Core/Binaries/` and `Plugins/SOTS_Core/Intermediate/` (if present) to clear stale artifacts.

VERIFIED
- None (no build/editor run performed).

UNVERIFIED / ASSUMPTIONS
- Assumption: `ModularFeatures` is not a valid UBT module name in this environment; including `Features/IModularFeatures.h` does not require a dedicated Build.cs dependency.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs
- Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251225_184900_FixModularFeaturesBuildCs.md
- Plugins/SOTS_Core/Docs/Anchor/Buddy_ContextAnchor_20251225_1849_FixModularFeaturesBuildCs.md

NEXT (Ryan)
- Re-run Editor build; success means the RulesError about `ModularFeatures` is gone and compilation continues.
- If a new missing module appears, capture the exact UBT line and target it similarly.

ROLLBACK
- Revert `Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs` to restore the dependency list (not recommended unless a new error indicates it’s required under a different module name).
