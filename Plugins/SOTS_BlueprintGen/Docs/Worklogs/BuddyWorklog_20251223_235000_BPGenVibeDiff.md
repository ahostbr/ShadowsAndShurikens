goal
- Compare the `SOTS_BPGenDiscovery` descriptor builder with `VibeUE BlueprintNodeService::DiscoverNodesWithDescriptors` to understand why the bridge assertion fires.

what changed
- Reviewed the descriptor construction flow in `SOTS_BPGenDiscovery.cpp`, noting that `AddPinDescriptorsFromSpawner` spawns a node inside `/Engine/Transient.BPGenDiscoveryTempGraph` to collect pin metadata.
- Examined `BlueprintNodeService.cpp` to see that Vibe uses `FBlueprintReflection::DiscoverNodesWithDescriptors`, which returns descriptors without invoking node spawners.

files changed
- None (analysis-only work).

notes + risks/unknowns
- Spawning variable nodes outside of a real Blueprint appears to hit `FBlueprintEditorUtils::FindBlueprintForNodeChecked` because the temporary graph lacks a blueprint context, so the editor asserts whenever a `UBlueprintVariableNodeSpawner` needs to resolve its owning Blueprint.
- Switching to reflection-based descriptors or otherwise providing a real Blueprint context might be required; this needs verification.

verification status
- Not verified (no build/editor run performed).

follow-ups / next steps
- Confirm whether `FSOTS_BPGenNodeSpawnerDescriptor` truly needs live nodes for pin data; if not, consider reusing `FBlueprintReflection` descriptors like Vibe does to avoid the transient graph crash.
- If live node data is essential, determine how to supply `UBlueprint` context or guard `Invoke` calls so that variable spawners skip detail gathering when the graph cannot resolve a Blueprint.
