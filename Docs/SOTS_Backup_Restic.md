# SOTS Local Snapshot Backup (restic)

This is a **local-only** snapshot backup system for the entire SOTS repo (including **`/Content`**) using **restic**.

- First run: create a **full** baseline snapshot.
- Later runs: create **push** snapshots (incremental/deduplicated).
- Every run prints progress and writes a per-run log file.

Note: restic snapshot **IDs are always random**. This tooling adds **auto-tags** (timestamp + optional note, and git branch/status when available) so snapshots are easy to find/describe like “auto commit messages”.

## Locations (canonical)

- Project root (default): `E:\SAS\ShadowsAndShurikens`
- Backup root: `C:\UE5\SOTS_Backup\`
  - Restic repo: `C:\UE5\SOTS_Backup\restic_repo`
  - Logs: `C:\UE5\SOTS_Backup\logs`
  - Passfile: `C:\UE5\SOTS_Backup\.restic_pass.txt`

## One-time setup

### 1) Create the passfile

Create this file once:
- `C:\UE5\SOTS_Backup\.restic_pass.txt`

Contents example (put your own long random password):

```text
use a long random password
```

### 2) Install restic (if needed)

If `restic version` fails, install restic and retry.
- winget (example): `winget install restic.restic`
- choco (example): `choco install restic`

## First full backup (baseline)

From repo root:

```powershell
python DevTools/python/sots_backup/full_sots_backup.py
```

Optional note (adds a tag like `note_<...>`):

```powershell
python DevTools/python/sots_backup/full_sots_backup.py --note "baseline before refactor"
```

What it does:
- Creates `C:\UE5\SOTS_Backup\{restic_repo,logs}` if missing
- Validates restic + passfile
- Runs `restic init` if repo is not initialized
- Runs a full snapshot with tag `full`

## Push backups (incremental snapshots)

### VS Code

Run the task labeled exactly:
- **push sots_backup**

### Terminal

```powershell
python DevTools/python/sots_backup/push_sots_backup.py
```

Optional note:

```powershell
python DevTools/python/sots_backup/push_sots_backup.py --note "after UI pass"
```

After success it prints the last 3 snapshots.

## Verify backups

List snapshots:

```powershell
python DevTools/python/sots_backup/sots_backup.py snapshots
```

Filter by tag (examples):

```powershell
restic -r C:\UE5\SOTS_Backup\restic_repo --password-file C:\UE5\SOTS_Backup\.restic_pass.txt snapshots --tag full
restic -r C:\UE5\SOTS_Backup\restic_repo --password-file C:\UE5\SOTS_Backup\.restic_pass.txt snapshots --tag ts_20251225_224000
```

Diff last two snapshots:

```powershell
python DevTools/python/sots_backup/sots_backup.py diff
```

## Restore latest snapshot

Restore latest snapshot into a target folder:

```powershell
python DevTools/python/sots_backup/restore_latest_sots_backup.py --target "D:\\SOTS_Restore_Test"
```

## Retention (forget/prune)

Example retention policy:

```powershell
python DevTools/python/sots_backup/forget_prune_sots_backup.py --keep-last 10 --keep-weekly 8 --keep-monthly 12
```

## Configuration

Edit:
- `DevTools/python/sots_backup/sots_backup_config.json`

Source root override options:
- Env var: `SOTS_PROJECT_ROOT`
- CLI: `--src "E:\\SAS\\ShadowsAndShurikens"`

Excludes:
- MUST-excluded by default (regen): `DerivedDataCache`, `Intermediate`, `Saved`, `.vs`
  - Opt-in to include them: set `excludes.regen_excludes_enabled=false`
- SHOULD-excluded by default (optional): `Binaries`, `Plugins/*/Intermediate`, `Plugins/*/Binaries`
  - Opt-in to include them: set `excludes.optional_excludes_enabled=false`

Notes:
- `/Content` is **not excluded** and is included in all snapshots.
- The scripts refuse to use `DevTools\\python\\sots_mcp_server` as a `--src` root.
