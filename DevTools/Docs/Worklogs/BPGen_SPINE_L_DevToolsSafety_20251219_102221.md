# BPGen SPINE L - DevTools Safety (2025-12-19)

## Summary
- Updated replay runner to support dangerous ops opt-in and auth tokens.
- Added prompt topic for safe mode, dangerous ops, and audit logs.

## DevTools changes
- `DevTools/python/bpgen_replay_runner.py`
  - `--dangerous-ok` flag to set `dangerous_ok=true`.
  - `--auth-token` option to override `SOTS_BPGEN_AUTH_TOKEN`.
- `DevTools/prompts/BPGen/topics/safety-and-audit.md`
  - Guidance on safe mode, dangerous ops, and audit log usage.
- `DevTools/prompts/BPGen/README.md`
  - Added safety topic index entry.

## Notes
- No build/run performed (code-only verification).
