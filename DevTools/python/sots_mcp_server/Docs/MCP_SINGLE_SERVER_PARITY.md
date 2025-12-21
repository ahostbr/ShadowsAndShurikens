# SOTS MCP Single Server Parity (Buddy reference)

## Overview
- Single MCP entrypoint: `DevTools/python/sots_mcp_server/server.py` (stdio).
- Canonical tools live on this server; legacy names are aliased (see `sots_help`).
- Mutations gated by env:
  - `SOTS_ALLOW_APPLY` (legacy, base gate)
  - `SOTS_ALLOW_BPGEN_APPLY` (defaults to `SOTS_ALLOW_APPLY`)
  - `SOTS_ALLOW_DEVTOOLS_RUN` (defaults allow; read-only scripts always safe)

## Preflight
1) `sots_help` (discover tools, aliases, gates, roots)
2) `sots_smoketest_all` (safe checks: server_info, agents_health, bpgen_ping)

## Canonical tools (selected)
- Workspace: `sots_list_dir`, `sots_read_file`, `sots_search`, `sots_search_workspace`, `sots_search_exports`
- Reports: `sots_list_reports`, `sots_read_report`
- DevTools: `sots_run_devtool` (gated by `SOTS_ALLOW_DEVTOOLS_RUN`)
- Agents: `sots_agents_health`, `sots_agents_run`, `sots_agents_help`, `agents_run_prompt`, `agents_server_info`
- BPGen: `bpgen_ping`, `manage_bpgen`, `bpgen_discover/list/describe/refresh/compile/save/apply/delete_node/delete_link/replace_node/ensure_function/ensure_variable/get_spec_schema/canonicalize_spec` (mutations gated; `dry_run` supported on apply/delete/replace/ensure)
- Diagnostics: `sots_last_error`, `sots_env_dump_safe`, `sots_where_am_i`

## Aliases
- `agents_run_prompt` -> `sots_agents_run`
- `agents_server_info` -> `sots_agents_help`
- `manage_bpgen` -> thin BPGen forwarder (alias for `bpgen_call`)
- `sots_search_workspace` / `sots_search_exports` -> `sots_search` with preset roots

## Logging / Safety
- No stdout logging (MCP stdio-safe). JSONL logs at `DevTools/python/logs/sots_mcp_server.jsonl` (redacted).
- Reports root: `DevTools/python/reports` (read-only).
- BPGen forwarding via TCP bridge (`sots_bpgen_bridge_client.py`); mutations blocked when gates are off.

## Prompt Pack Guidance
- Use unified tools; avoid referencing legacy separate servers.
- For BPGen prompts: canonicalize → dry_run (optional) → apply → list/describe → compile/save.
