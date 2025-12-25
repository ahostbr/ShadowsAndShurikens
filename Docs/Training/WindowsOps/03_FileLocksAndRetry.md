# File Locks + Retry Rules

- Windows commonly locks files (AV/indexer/UE editor).
- For delete/move/copy operations, use retry with backoff.
- Prefer “copy then replace” rather than in-place overwrite when feasible.
- When retries exhaust, fail with actionable path info.
