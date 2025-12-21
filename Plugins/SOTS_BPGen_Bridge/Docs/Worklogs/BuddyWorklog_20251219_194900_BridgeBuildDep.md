# Buddy Worklog — Bridge build dependency restore

## goal
Fix bridge compile error (missing SOTS_BPGenBuilder.h) after removing plugin dependency by restoring the module dependency without reintroducing the plugin-level cycle.

## what changed
- Added `SOTS_BlueprintGen` back to `PublicDependencyModuleNames` in `SOTS_BPGen_Bridge.Build.cs` so bridge code can include BPGen headers.
- Left the uplugin dependency removed to avoid the previous plugin-level circular reference.

## files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.Build.cs

## notes + risks/unknowns
- Build not rerun; compile remains UNVERIFIED.
- If BlueprintGen is ever disabled, bridge will not compile; dependency is required for server routing.

## verification status
- Not built or run.

## follow-ups / next steps
- Re-run ShadowsAndShurikensEditor build to confirm compile passes and the circular plugin warning is gone.
