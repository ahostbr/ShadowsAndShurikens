goal
- Teach the bridge to accept `apply_function_skeleton` so we can create the EventGraph before wiring nodes.

what changed
- Added an `apply_function_skeleton` branch to SOTS_BPGenBridgeServer: JSON params are deserialized into `FSOTS_BPGenFunctionDef`, the builder runs `ApplyFunctionSkeleton`, and the result/errors/warnings are marshaled back to the requester.
- Reran WORKING/run_bpgen_notification_job.py after the change; the job still sees "Unknown action 'apply_function_skeleton'" because the running bridge binary needs to be rebuilt with this update, so the later `apply_graph_spec` call never fires.

files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- DevTools/python/logs/sots_mcp_server.jsonl

notes + risks/unknowns
- Until the plugin is rebuilt/restarted, clients will keep hitting the unknown-action error and cannot create the EventGraph that `apply_graph_spec` expects.
- The Blueprint creation step now succeeds (reports existing asset), so any future job will only fail on `apply_function_skeleton` until the bridge is updated.

verification status
- UNVERIFIED (bridge still missing the handler in the running binary; `apply_graph_spec` keeps failing because the graph is not created).

follow-ups / next steps
- Build/restart SOTS_BPGen_Bridge so the new handler is available, rerun the notification job with `apply_function_skeleton` + `apply_graph_spec`, then confirm the "Merry Christmas Ryan !" notification appears via SOTS UI/ProHUD.
