# BPGen Discovery-First Workflow (SPINE_D)

1) Discover nodes → get `spawner_key` (SPINE_A)
   - Call your discovery API to search by text/category and return descriptors (spawner_key + FunctionPath + pins).
2) Build GraphSpec nodes from descriptors
   - Use `SpawnerKey` on `FSOTS_BPGenGraphNode` (set `bPreferSpawnerKey=true`).
   - Always carry `FunctionPath` for function/event nodes.
   - `NodeType` is legacy and only valid for synthetic nodes (entry/result/knot/select/dynamic-cast) during a short sunset; plan to drop it in new specs.
3) Connect pins schema-first (SPINE_C)
   - Links now accept `bUseSchema` (default true) and optional break flags.
4) Apply
   - `ApplyFunctionSkeleton` (ensure function exists) → `ApplyGraphSpecToFunction`.
5) Verify (SPINE_E maintenance helpers)
   - Use list/describe/compile/save/refresh helpers (SPINE_E) as the standard surface.
   - Older apply-only flows are deprecated and will be removed once MCP/bridge routes exclusively through the maintenance helpers.

Example (PrintString via spawner key)
- SpawnerKey: `/Script/Engine.KismetSystemLibrary:PrintString`
- Graph:
  - Node: `PrintString` with SpawnerKey + NodeType `K2Node_CallFunction`
  - ExtraData: `InString.DefaultValue = "Hello from BPGen"`
  - Links: `Entry.then -> PrintString.execute`, `PrintString.then -> Result.execute`
- Helper: `USOTS_BPGenExamples::Example_ApplyPrintStringGraph` builds and applies this graph.

Best practices
- Always prefer `SpawnerKey` and include `FunctionPath`; specs that omit `SpawnerKey` will break when the short NodeType sunset ends.
- Keep `NodeType` only on the synthetic set (entry/result/knot/select/dynamic-cast) while the sunset is active; remove it elsewhere.
- Use `bUseSchema=true` unless you explicitly need raw `MakeLinkTo`.
- Keep `NodeId` stable across edits so future SPINE_E list/describe can diff.
- NodeId is the single identity field; do not invent parallel IDs. Storage is currently via NodeComment and will move to a dedicated metadata slot later without changing the field name.
- Avoid including pins in discovery unless necessary (costly); request pins only when you need shape hints.
