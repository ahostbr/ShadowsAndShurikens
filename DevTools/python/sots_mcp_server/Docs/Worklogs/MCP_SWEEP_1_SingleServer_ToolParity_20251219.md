## MCP Sweep 1: Single-Server Tool Parity (2025-12-19)

- Files touched: `server.py`, `Docs/Worklogs/MCP_SWEEP_1_SingleServer_ToolParity_20251219.md`.
- Added core workspace tools on unified server: `sots_list_dir`, `sots_read_file`, `sots_search_workspace`, `sots_search_exports` (root-keyed, read-only).
- Added BPGen parity wrappers: `manage_bpgen`, `bpgen_discover`, `bpgen_list`, `bpgen_describe`, `bpgen_refresh`, `bpgen_compile`, `bpgen_save`, `bpgen_apply`, `bpgen_delete_node`, `bpgen_delete_link`, `bpgen_replace_node`, `bpgen_ensure_function`, `bpgen_ensure_variable`, `bpgen_get_spec_schema`, `bpgen_canonicalize_spec`; mutations block when `SOTS_ALLOW_APPLY=0` before bridge call.
- Agents tools retained and aliased: `sots_agents_*` plus `agents_run_prompt` noted in aliases.
- Server info now advertises roots, gates, and tool_aliases; logging remains file/stderr only.
- Assumptions: exports root mapped to `DevTools/prompts/BPGen` for search/list/read; no tests run.
