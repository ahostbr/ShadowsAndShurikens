# BPGen SPINE_M Canonicalize + Migrate + Repair (2025-12-19 11:17:40)

Summary
- Added GraphSpec `spec_version`/`spec_schema` with canonicalization API + deterministic sort.
- Added data-driven migration maps for pin aliases, node class changes, and function path drift.
- Apply now supports `repair_mode` with `repair_steps` reporting.
- Bridge exposes `get_spec_schema` and `canonicalize_spec` actions.

Notes
- Canonicalization ordering mirrors SPINE_I inspector sort rules.
- Migration alias patterns follow VibeUE pin alias handling (CommonUtils).
