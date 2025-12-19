Goal
- SPINE_D: add discovery-first translator + example harness + workflow doc.

What changed
- Added descriptor→GraphNode translator helper (BlueprintPure) to map spawner descriptors into BPGen node specs.
- Added simple example harness that applies a PrintString graph via spawner key and schema-first linking.
- Added discovery-first workflow doc summarizing the loop (discover → spawner key → build spec → apply → verify).

Files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenDescriptorTranslator.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDescriptorTranslator.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenExamples.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenExamples.cpp
- Plugins/SOTS_BlueprintGen/Docs/BPGen_DiscoveryFirst_Workflow.md

Notes / risks / unknowns
- Example harness assumes PrintString exec pins: Entry.then -> PrintString.execute -> Result.execute; unverified at runtime.
- Translator sets NodeType from NodeClassName or NodeType string if present; pin defaults/extra hints are minimal.
- No build/run performed.

Verification status
- UNVERIFIED (no build/run; editor-only changes).

Cleanup
- Pending (delete Binaries/Intermediate after edits).

Follow-ups / next steps
- Add NodeId-based inspection/list/describe (SPINE_E).
- Expand translator to map more descriptor fields (pins, container types) as needed.
- Consider a discovery-driven example that actually calls the discovery API once implemented.
