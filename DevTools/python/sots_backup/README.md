# SOTS Backup (restic, local)

This folder provides a **local-only** snapshot backup system for the full SOTS repo **including `/Content`**.

**Key idea**
- Run a one-time **full** baseline snapshot.
- Later, run **push** snapshots (incremental/dedup) whenever you want.
- Every run streams progress to console **and** writes a log file.

Note: restic snapshot IDs are random. This tooling adds extra **tags** (timestamp + optional note, and git branch/status when available) so snapshots are easier to find/describe.

## Quick commands

One-time full baseline:

```powershell
python DevTools/python/sots_backup/full_sots_backup.py
```

Incremental “push” snapshot:

```powershell
python DevTools/python/sots_backup/push_sots_backup.py
```

Optional note (adds a tag like `note_<...>`):

```powershell
python DevTools/python/sots_backup/push_sots_backup.py --note "after lighting tweaks"
```

List snapshots:

```powershell
python DevTools/python/sots_backup/sots_backup.py snapshots
```

Diff last two snapshots:

```powershell
python DevTools/python/sots_backup/sots_backup.py diff
```

Restore latest to a target folder:

```powershell
python DevTools/python/sots_backup/restore_latest_sots_backup.py --target "D:\\SOTS_Restore_Test"
```

Retention (example):

```powershell
python DevTools/python/sots_backup/forget_prune_sots_backup.py --keep-last 10 --keep-weekly 8 --keep-monthly 12
```

## Logs

Logs are written per run to:
- `C:\UE5\SOTS_Backup\logs\sots_backup_<command>_YYYYMMDD_HHMMSS.log`

Console output mirrors the log line-by-line.

## VS Code task alias

When the user says exactly: **push sots_backup**
- Run the VS Code task labeled **push sots_backup**
- Or run the CLI:
  - `python DevTools/python/sots_backup/push_sots_backup.py`

## Configuration

Edit `DevTools/python/sots_backup/sots_backup_config.json`.
- Default source root: `E:\\SAS\\ShadowsAndShurikens`
- Override source root via env var `SOTS_PROJECT_ROOT` or CLI `--src`
- Regen folders are excluded by default, but you can opt in by setting `excludes.regen_excludes_enabled=false`.
- `Binaries` / `Plugins/*/(Binaries|Intermediate)` are excluded by default, but you can opt in by setting `excludes.optional_excludes_enabled=false`.

See `Docs/SOTS_Backup_Restic.md` for the full workflow.
