# Buddy Worklog — Snapshot auto-tags (restic)

Date: 2025-12-25
Owner: Buddy

## Goal
Improve snapshot “naming” so backups are discoverable like auto-generated commit messages + timestamp.

## What changed
- Added automatic tagging for `full` and `push` backups:
  - always adds `ts_YYYYMMDD_HHMMSS`
  - optionally adds `note_<...>` via `--note "..."`
  - when a git repo is present and `git` is available:
    - adds `git_<branch>`
    - adds `dirty_s<staged>_u<unstaged>` derived from `git status --porcelain`
- Updated wrapper scripts so `push_sots_backup.py` and `full_sots_backup.py` accept `--note`, `--src`, `--config`.
- Updated docs to explain tags vs random snapshot IDs.

## Files changed
- DevTools/python/sots_backup/sots_backup.py
- DevTools/python/sots_backup/push_sots_backup.py
- DevTools/python/sots_backup/full_sots_backup.py
- DevTools/python/sots_backup/README.md
- Docs/SOTS_Backup_Restic.md

## Verification status
- VERIFIED: Ran `push_sots_backup.py --note "test_note_tagging"` and observed tags applied in snapshot metadata.

## Notes / Risks
- Snapshot IDs remain random (restic design). Tags are the supported way to attach human-readable labels.
- Tag sanitization is conservative (non [A-Za-z0-9._-] becomes `_`, capped length).
