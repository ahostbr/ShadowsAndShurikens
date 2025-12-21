## MCP Sweep 3: Gating Matrix & Dry-Run (2025-12-19)

- Added gating matrix envs: `SOTS_ALLOW_BPGEN_APPLY` (inherits `SOTS_ALLOW_APPLY`), `SOTS_ALLOW_DEVTOOLS_RUN` (defaults to allow; read_only script metadata can bypass when off).
- DevTools runner now blocks when gate is off unless script metadata marks `read_only`; still no stdout logging.
- BPGen mutating wrappers now support `dry_run` payload returns and continue to honor gates before bridge calls.
- Response envelope helpers applied with request_id + JSONL logging.
- Work remains read-only defaults; no tests run.
