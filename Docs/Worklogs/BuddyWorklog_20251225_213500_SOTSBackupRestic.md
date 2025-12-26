# Buddy Worklog — SOTS local snapshot backup (restic)

Date: 2025-12-25
Owner: Buddy

## Goal
Implement a local snapshot backup system for the full SOTS repo (including `/Content`) with:
- one-time full baseline snapshot
- incremental/dedup push snapshots on demand
- per-run log files + live console progress
- VS Code task alias labeled exactly: "push sots_backup"

## What changed
- Added a new `DevTools/python/sots_backup` tool folder with:
  - shared CLI + helpers (`sots_backup.py`)
  - entrypoints for full/push
  - restore-latest + forget/prune helpers
  - editable JSON config
- Added user-facing docs describing setup and workflow.
- Added VS Code task to run the push command from repo root.

## Files added/edited
- DevTools/python/sots_backup/sots_backup.py
- DevTools/python/sots_backup/full_sots_backup.py
- DevTools/python/sots_backup/push_sots_backup.py
- DevTools/python/sots_backup/restore_latest_sots_backup.py
- DevTools/python/sots_backup/forget_prune_sots_backup.py
- DevTools/python/sots_backup/sots_backup_config.json
- DevTools/python/sots_backup/README.md
- Docs/SOTS_Backup_Restic.md
- .vscode/tasks.json

## Notes / Risks / Unknowns
- UNVERIFIED: restic CLI presence on PATH on this machine; scripts print a friendly install hint if missing.
- UNVERIFIED: restic init/backup/restore execution (per instructions: no Unreal build/run; restic not invoked here).
- Excludes are implemented as absolute paths built from the configured project root, and plugin Binaries/Intermediate excludes are expanded by enumerating `Plugins/*`.

## Verification status
- UNVERIFIED: end-to-end restic backup/restore (not executed).
- VERIFIED (static): scripts write logs to `C:\UE5\SOTS_Backup\logs` and stream subprocess output line-by-line.

## Next steps (Ryan)
- Create passfile `C:\UE5\SOTS_Backup\.restic_pass.txt`.
- Run `python DevTools/python/sots_backup/full_sots_backup.py` once and confirm snapshot created.
- Use VS Code task "push sots_backup" to create incremental snapshots.
- Test restore to a temp folder via `restore_latest_sots_backup.py --target ...`.
