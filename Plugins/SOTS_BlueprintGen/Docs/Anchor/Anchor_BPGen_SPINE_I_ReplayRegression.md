[CONTEXT_ANCHOR]
ID: 20251219_120000 | Plugin: SOTS_BlueprintGen | Pass/Topic: SPINE_I_ReplayRegression | Owner: Buddy
Scope: Protocol versioning + deterministic inspector + graph edit helpers + replay harness

DONE
- Bridge responses add protocol_version, features, and error_code wiring; protocol mismatch returns ERR_PROTOCOL_MISMATCH
- Added delete_node/delete_link/replace_node helpers with transactions and bridge endpoints
- Inspector outputs sorted (nodes, pins, links, pin defaults) for deterministic JSON
- Added bpgen_replay_runner.py and six NDJSON replay packs + placeholder expected reports
- Added replay/regression prompt doc

VERIFIED
- None (code-only pass; no editor/build)

UNVERIFIED / ASSUMPTIONS
- New graph edit helpers compile and behave as expected
- Replay NDJSON files align with live asset paths

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGenBridgeServer.h
- Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenBuilder.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenBuilder.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenTypes.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenInspector.cpp
- DevTools/python/bpgen_replay_runner.py
- DevTools/prompts/BPGen/topics/replay-and-regression.md
- DevTools/replays/BPGen/*

NEXT (Ryan)
- Run replays with live bridge, capture reports into *_expected.json
- Validate delete/replace actions on a sample function and confirm deterministic inspector ordering
- Clean plugin Binaries/Intermediate before build

ROLLBACK
- Revert modified BPGen bridge/builder/inspector files and remove new DevTools replay assets if needed; no migrations required
