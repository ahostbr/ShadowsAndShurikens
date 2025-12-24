Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251223_235000 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenVibeDiff | Owner: Buddy
Scope: Compare our descriptor pipeline to VibeUE's reflection-based discovery to explain the transient-graph assertion.

DONE
- Read `SOTS_BPGenDiscovery.cpp` and noted that `AddPinDescriptorsFromSpawner` spawns nodes in `/Engine/Transient.BPGenDiscoveryTempGraph` to build pin descriptors.
- Reviewed `BlueprintNodeService::DiscoverNodesWithDescriptors` and confirmed Vibe uses `FBlueprintReflection::DiscoverNodesWithDescriptors` instead of spawning nodes, so it never hits `FindBlueprintForNodeChecked` on a transient graph.
- Logged the reasoning in `Docs/Worklogs/BuddyWorklog_20251223_235000_BPGenVibeDiff.md`.

VERIFIED
- None (analysis-only).

UNVERIFIED / ASSUMPTIONS
- The assertion is triggered because variable node spawners need a real Blueprint reference, and our transient graph does not provide it.
- Reusing reflection descriptors should sidestep the issue if the data we need is already available there.

FILES TOUCHED
- Docs/Worklogs/BuddyWorklog_20251223_235000_BPGenVibeDiff.md

NEXT (Ryan)
- Decide whether to stop invoking node spawners for pin metadata and instead consume `FBlueprintReflection` descriptors, or else supply a Blueprint context/override to the transient graph before calling `Invoke`.
- If reflection is insufficient, determine where the graph is expected to live so `FindBlueprintForNodeChecked` succeeds and add checks before invoking variable spawners.

ROLLBACK
- Delete `Docs/Worklogs/BuddyWorklog_20251223_235000_BPGenVibeDiff.md` and this anchor file.
