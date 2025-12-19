Goal
- SPINE_B: allow BPGen graph nodes to spawn via stable SpawnerKey with fallback to existing NodeType path.

What changed
- Added SpawnerKey + bPreferSpawnerKey to FSOTS_BPGenGraphNode for descriptor-driven node creation.
- Introduced minimal FSOTS_BPGenSpawnerRegistry (function-path keys only) with cache + clear helper.
- Added spawner-based spawn path in ApplyGraphSpecToFunction; attempts spawner first, falls back to legacy NodeType switch without behavior removal.

Files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenSpawnerRegistry.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenSpawnerRegistry.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp

Notes / risks / unknowns
- Registry currently only supports UFunction path keys; variable/cast/action spawners remain unsupported (will fall back to NodeType or warn).
- No automation/build executed; compile required.
- Spawner Invoke uses default empty bindings; variable nodes may need richer bindings in later passes.

Verification status
- UNVERIFIED (no build/run).

Cleanup
- Pending after edits (Binaries/Intermediate to delete).

Follow-ups / next steps
- Extend registry to handle variable/cast/custom spawners with context-specific bindings.
- Add cache clear exposure if needed by tooling; consider per-blueprint/context priming.
- Proceed to SPINE_C (schema-first linking) and SPINE_E/F/G for inspection + remote bridge + MCP surface.
