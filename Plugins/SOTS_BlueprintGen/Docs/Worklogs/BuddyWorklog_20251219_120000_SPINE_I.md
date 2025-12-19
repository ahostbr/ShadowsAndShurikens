# Buddy Worklog — SPINE_I Replay + Protocol Hardening

## Goal
Implement SPINE_I requirements: protocol/version flags, deterministic inspector output, delete/replace primitives, replay runner, and golden replays.

## What changed
- Added protocol_version + feature flags and protocol guardrails to BPGen bridge responses.
- Added graph edit helpers (delete node, delete link, replace node) with transactions and bridge exposure.
- Sorted inspector outputs (nodes, pins, links) for deterministic JSON.
- Added Python replay runner + six golden NDJSON replays with placeholder expectations.
- Documented replay/regression workflow.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGenBridgeServer.h
- Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md
- Plugins/SOTS_BlueprintGen/Source/SOTS_BPGen_Builder/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BPGen_Builder/Public/SOTS_BPGenBuilder.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BPGen_Builder/Public/SOTS_BPGenTypes.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BPGen_Inspector/Private/SOTS_BPGenInspector.cpp
- DevTools/python/bpgen_replay_runner.py
- DevTools/python/sots_bpgen_bridge_client.py (read)
- DevTools/prompts/BPGen/topics/replay-and-regression.md
- DevTools/replays/BPGen/*

## Notes / Risks / Unknowns
- New graph edit helpers are additive but unverified in-editor.
- Replay NDJSON files use placeholder asset paths; expect to update expectations after first run.
- Replace-node wiring relies on pin name matching; complex remaps may need follow-up.

## Verification
- Not run (no editor/build in this pass).

## Follow-ups / Next steps
- Run replays against live bridge, capture expected reports, and validate diffs.
- Consider adding request size/rate/auth guards from SPINE_I I6 if needed.
- Delete Binaries/Intermediate for touched plugins before build.
