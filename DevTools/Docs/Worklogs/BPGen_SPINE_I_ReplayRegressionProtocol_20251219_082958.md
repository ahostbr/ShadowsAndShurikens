# BPGen SPINE I - Replay Regression Protocol (2025-12-19)

Changes
- Bridge responses now include dry_run previews and guardrails (ERR_UNAUTHORIZED, ERR_RATE_LIMIT, ERR_REQUEST_TOO_LARGE).
- MCP server exposes delete/replace wrappers; DevTools client auto-injects auth token.
- Replay/regression topic updated for auth-token guidance.

Deterministic ordering
- Inspector outputs remain sorted (node, pin, link ordering) to keep replay diffs stable.

Replay runner example
- python DevTools/python/bpgen_replay_runner.py --replay DevTools/replays/BPGen/01_function_print_string.jsonl --out DevTools/reports/BPGen/01_function_print_string_report.json --diff DevTools/reports/BPGen/01_function_print_string_diff.txt --expect DevTools/replays/BPGen/01_function_print_string_expected.json

Golden replays
- 01_function_print_string
- 02_eventgraph_custom_event
- 03_macro_create_and_use
- 04_construction_script_patch
- 05_umg_widget_tree_and_binding
- 06_refactor_by_node_id
