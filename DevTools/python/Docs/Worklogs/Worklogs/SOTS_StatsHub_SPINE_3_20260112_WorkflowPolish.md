# Worklog: SOTS Stats Hub (SPINE_3 Workflow Polish)
- Added global Command Palette (Ctrl+K) for tab navigation and common actions (reports, docs zip, zips, open folders, plugin jump).
- Introduced JobManager + Jobs tab: serial queue, cancel, per-job logs under `_reports/sots_stats_hub/JobLogs`, history view.
- Routed report generation, zips, and docs combo zip through the job queue with streaming logs to UI + log files.
- Added smart report generation toggle/stale-minutes, docs combo zip (scan/zip/ack), and palette recent memory.
- Updated README with palette, jobs, smart reports, and settings.

## Verify
- `python -m sots_stats_hub`
- Command Palette (Ctrl+K) navigates and runs actions; recent list updates.
- Reports tab: Generate Smart/All enqueue jobs; status cards refresh on change; job logs written.
- Docs Watch: Combo Zip enqueues job, zips new/modified, ACKs baseline; lists refresh.
- Jobs tab: shows active queue/history; Cancel works; logs stored.
