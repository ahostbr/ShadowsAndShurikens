# BPGen_SPINE_H_CoreTargetsEnsure_20251219_150500

goal
- Deliver SPINE_H core (non-UMG): target-aware graph apply, ensure primitives surfaced, bridge routing, edit lock/transactions. UMG deferred to SPINE_H2 per instruction.

what changed
- Added global BPGen edit mutex definition to serialize ensure/apply flows.
- Resolved duplicate apply implementation by keeping target-aware entrypoints and preserving legacy body as an internal helper.
- EnsureFunction now applies pure/const flags on the entry node based on signature.
- Bridge server gained actions: apply_graph_spec_to_target, ensure_function, ensure_variable; documented in protocol.
- SPINE checklist annotated that SPINE_H core is done and UMG is deferred to SPINE_H2.

files changed
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenEnsure.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md
- Plugins/SOTS_BlueprintGen/Docs/BPGEN_121825_NEEDEDJOBS.txt

notes + risks/unknowns
- Legacy apply body retained as a static helper; unused but left for reference. Target-aware path is the active one.
- Pure/const flags are applied via EntryNode ExtraFlags; deeper UFunction flag sync not yet validated.
- UMG/WidgetBinding targets and ensure_widget/binding actions intentionally deferred to SPINE_H2.
- No runtime/editor verification performed; bridge/MCP round-trip not exercised.

verification status
- UNVERIFIED (no build/run; code-only changes)

follow-ups / next steps
- Implement SPINE_H2 for UMG targets and widget/binding ensure actions.
- Consider adding dispatcher/interface ensure helpers if needed for parity.
- Exercise bridge actions against live UE session to confirm JSON contracts.
