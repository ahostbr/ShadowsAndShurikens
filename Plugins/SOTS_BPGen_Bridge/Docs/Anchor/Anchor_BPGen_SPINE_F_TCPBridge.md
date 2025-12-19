[CONTEXT_ANCHOR]
ID: 20251219_0001 | Plugin: SOTS_BPGen_Bridge | Pass/Topic: SPINE_F_TCPBridge | Owner: Buddy
Scope: Local TCP NDJSON bridge to route BPGen actions from external tools.

DONE
- Added Editor plugin SOTS_BPGen_Bridge with module + bridge server
- Implemented TCP NDJSON server (127.0.0.1:55557) with ping/apply_graph_spec/shutdown actions
- Game-thread dispatch wrapper and JSON response builder
- Protocol doc and worklog added

VERIFIED
- None (no build/editor run)

UNVERIFIED / ASSUMPTIONS
- JSON-to-struct conversion for apply_graph_spec matches BPGen structs in practice
- Timeouts and sequential handling are sufficient for tooling use

FILES TOUCHED
- Plugins/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.uplugin
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/*
- Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md
- Plugins/SOTS_BPGen_Bridge/Docs/Worklogs/BPGen_SPINE_F_TCPBridge_20251219_000100.md

NEXT (Ryan)
- Build the plugin to verify server compiles and loads in editor
- Manually Start server and issue ping/apply_graph_spec over TCP to confirm round-trip
- Extend actions once SPINE_E inspection APIs exist

ROLLBACK
- Remove Plugins/SOTS_BPGen_Bridge entirely (uplugin + Source + Docs)
