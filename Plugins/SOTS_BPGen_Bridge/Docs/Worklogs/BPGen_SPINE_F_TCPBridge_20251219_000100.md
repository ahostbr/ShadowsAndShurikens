# Buddy Worklog — SPINE_F TCP Bridge

goal
- add a local-only TCP NDJSON bridge so external tools can drive BPGen actions

what changed
- new Editor plugin SOTS_BPGen_Bridge with bridge server and module skeleton
- TCP server listens on 127.0.0.1:55557 (configurable via Start parameters), sequential connections, newline-delimited JSON
- requests dispatch onto game thread; apply_graph_spec routes into USOTS_BPGenBuilder::ApplyGraphSpecToFunction
- actions ping and shutdown implemented; other actions return explicit not-implemented errors
- protocol doc added describing framing, request/response, supported actions

files changed
- Plugins/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.uplugin
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.Build.cs
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGen_BridgeModule.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGen_BridgeModule.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGenBridgeServer.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md

notes + risks/unknowns
- discover/list/describe/compile/save/refresh actions are stubbed (return not implemented); upstream BPGen APIs for these are still pending
- socket read and game-thread wait have fixed timeouts; no config surface yet
- server is not auto-started; Start must be called manually (module retains server singleton only)
- JSON conversion for apply_graph_spec relies on field names matching BPGen structs; error handling is minimal

verification status
- UNVERIFIED (no build/editor run)

cleanup
- No Binaries/Intermediate folders created for SOTS_BPGen_Bridge at this stage

follow-ups / next steps
- Implement missing actions once SPINE_E surfaces exist (discover/list/describe/compile/save/refresh)
- Add developer settings (auto-start, bind address/port, max bytes)
- Consider length-prefix framing fallback and structured error codes
