# BPGen SPINE_M Spec Tools (2025-12-19 11:17:40)

Summary
- Added `bpgen_spec_upgrade.py` to canonicalize/migrate specs via bridge.
- Added `bpgen_spec_lint.py` to validate required fields and warn on risky patterns.
- MCP server exposes `bpgen_canonicalize_spec` and `bpgen_get_spec_schema`.

Notes
- Upgrade tool writes `<name>_upgraded.json` and `<name>_upgrade_report.json`.
- Lint flags missing node_id and heuristic pin matching (risk).
