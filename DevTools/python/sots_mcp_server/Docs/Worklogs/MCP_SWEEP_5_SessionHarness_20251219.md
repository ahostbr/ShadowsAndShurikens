## MCP Sweep 5: Session Harness Helpers (2025-12-19)

- Added read-only helpers: `sots_smoketest_all` (aggregates server_info, agents_health, bpgen_ping), `sots_env_dump_safe` (env presence booleans), and `sots_where_am_i` (paths + branch probe).
- New tools use envelopes, request_id, and JSONL logging; no stdout.
- `sots_help` canonical list updated; no mutations or tests performed.
