Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251224_000500 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenReflectionDiscovery | Owner: Buddy
Scope: Swap our discovery pin-gathering path to use VibeUE’s reflection descriptors so the transient graph never explodes.

DONE
- Added reflection conversion helpers that map `FBlueprintReflection::FNodeSpawnerDescriptor` + `FPinDescriptor` to `FSOTS_BPGenNodeSpawnerDescriptor` and `FSOTS_BPGenDiscoveredPinDescriptor`.
- Rerouted `DiscoverNodesWithDescriptors` to call `FBlueprintReflection` when a Blueprint asset is provided and to skip pin collection entirely in the fallback action registry mode.
- Declared `VibeUE` as a dependency so the reflection headers compile within SOTS_BlueprintGen.

VERIFIED
- None (no build or editor run yet).

UNVERIFIED / ASSUMPTIONS
- Reflection descriptors supply the same fields and keywords that our bridge clients expect, so pin metadata remains stable.
- The fallback discovery loop still satisfies searches when no Blueprint path is given even without pin descriptors.

FILES TOUCHED
- Source/SOTS_BlueprintGen/Private/SOTS_BPGenDiscovery.cpp
- Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs

NEXT (Ryan)
- Restart the editor, rerun the discovery payload for `BP_SOTS_Player`, and confirm there is no `FindBlueprintForNodeChecked` assertion; pair the log with the payload to prove the break is resolved.
- If fallback runs without pins, evaluate whether to proxy reflection descriptors even without a blueprint or to emit a warning so callers request a context asset.

ROLLBACK
- Revert the modified SOTS_BPGenDiscovery.cpp and Build.cs files (or delete this anchor/worklog entry) to restore the previous transient-graph behavior.
