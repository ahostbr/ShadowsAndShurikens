# BPGen Prompt Pack (SPINE_G)

Use with `SOTS_BPGen` FastMCP server (DevTools/python/sots_bpgen_mcp_server.py) that forwards to the UE TCP bridge.

Included topics
- topics/overview.md — loop and ground rules.
- topics/blueprint-workflow.md — step-by-step tool usage.
- topics/recovery-and-verification.md — error handling and verification checklist.
- topics/autofix-and-conversions.md - auto-fix flags, steps, and conversion guidance.
- topics/batch-and-sessions.md - batch/session usage, atomic semantics, and orchestration tips.
- topics/safety-and-audit.md - auth tokens, dangerous_ok, safe mode, and audit logs.
- topics/replay-and-regression.md - replay runner usage, golden replays, and diff interpretation.

Examples
- examples/01_print_string_function.md — minimal function graph apply + verify.
- examples/02_ensure_var_set_branch.md — variable + set + branch chain.
- examples/03_widget_bind_text.md — widget binding (assuming bridge supports it).
- examples/05_autofix_schema_reject_insert_conv.md - type mismatch auto-fix + conversion insertion.

Usage tips
- Prefer `manage_bpgen` for flexibility; wrappers are thin convenience calls.
- Use graph edit wrappers (`bpgen_delete_node`, `bpgen_delete_link`, `bpgen_replace_node`) and `dry_run` for previews.
- Always verify with `bpgen_list`/`bpgen_describe` after `bpgen_apply`.
- Compile and save explicitly when edits matter (`bpgen_compile` + `bpgen_save`).
