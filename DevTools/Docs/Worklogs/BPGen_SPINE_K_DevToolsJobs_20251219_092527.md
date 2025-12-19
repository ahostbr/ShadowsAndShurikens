# BPGen SPINE K - DevTools Jobs (2025-12-19)

## Summary
- Added DevTools scripts for batch submission and job folder polling.
- Added prompt topic covering batch/session usage and recommended macro flow.

## DevTools additions
- `DevTools/python/bpgen_batch_submit.py`
  - Reads batch JSON, submits `batch` or `session_batch` to the bridge.
  - Writes a report JSON and prints summary output.
- `DevTools/python/bpgen_jobs_runner.py`
  - Polls `DevTools/Inbox/BPGenJobs` by default.
  - Submits jobs as batches, writes report/log files, moves jobs to Done/Failed.
- `DevTools/prompts/BPGen/topics/batch-and-sessions.md`
  - Guidance on batch semantics, sessions, and a golden workflow macro.

## Files touched
- `DevTools/python/bpgen_batch_submit.py`
- `DevTools/python/bpgen_jobs_runner.py`
- `DevTools/prompts/BPGen/topics/batch-and-sessions.md`
- `DevTools/prompts/BPGen/README.md`

## Notes
- Jobs runner writes reports to `DevTools/reports/BPGen`.
- No build/run performed (code-only verification).
