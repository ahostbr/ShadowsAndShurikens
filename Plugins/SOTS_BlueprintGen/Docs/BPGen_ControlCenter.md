# BPGen Control Center (SPINE_O)

Location: Editor tab `SOTS BPGen Control Center` under Window → SOTS Tools.

Features (v0.1):
- Bridge controls: start/stop the SOTS_BPGen_Bridge server.
- Status snapshot: running/bind/safe-mode flags, limits (bytes/RPS/RPM), uptime, and enabled features.
- Live monitoring: recent requests (newest first) with request id, ok/error code, duration ms, and timestamp.
- Debug tools: annotate nodes (visual highlight), clear BPGen annotations, focus a node by `NodeId`.
- Inputs: Blueprint asset path, function name, node id, annotation text (default `[BPGEN]`).

Notes:
- Annotations set an error bubble with `[BPGEN]` prefix and tint; they do **not** overwrite BPGen NodeId comments.
- Clear Annotations removes `[BPGEN]` bubbles and resets tint.
- Focus Node uses the Blueprint editor to jump to the node (best effort, editor-only).
- Recent requests are read from the bridge in-process via `GetRecentRequestsForUI`; status info uses `GetServerInfoForUI`.
