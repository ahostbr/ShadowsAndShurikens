goal
- Replace the transient graph pin-spawning discovery path with a reflection-based pipeline like VibeUE so the bridge never hits `FindBlueprintForNodeChecked`.

what changed
- Added `FBlueprintReflection` conversion helpers and now use its descriptor array whenever a Blueprint asset path is supplied, including pin metadata when requested.
- The fallback action-registry loop still enumerates spawners but no longer spawns nodes or registers pin descriptors, avoiding the transient graph assertion.
- Declared a build dependency on `VibeUE` so the reflection headers can be referenced safely.

files changed
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenDiscovery.cpp
- Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs

notes + risks/unknowns
- Reflection discovery requires a loaded Blueprint; if the bridge ever calls `discover_nodes` without a path the fallback metadata lacks pin details and may need future polish.
- The new conversion layer assumes the reflection descriptor already matches the fields the bridge clients expect; if missing data is discovered we may need to enrich the mapping.

verification status
- Not run (no editor/build invoked).

follow-ups / next steps
- Launch the editor, rerun `discover_nodes` for a real Blueprint, and confirm the formerly-broken assertion no longer fires.
- If the fallback path is exercised, document what metadata (especially pins) is missing and decide if reflection coverage needs backfilling.
