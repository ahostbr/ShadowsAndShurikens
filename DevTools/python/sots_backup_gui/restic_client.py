from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Callable


def _ts() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def make_log_path(log_dir: Path, action: str) -> Path:
    log_dir.mkdir(parents=True, exist_ok=True)
    return log_dir / f"sots_backup_gui_{action}_{_ts()}.log"


class ActionLogger:
    def __init__(self, log_path: Path, ui_callback: Callable[[str], None] | None = None) -> None:
        self.log_path = log_path
        self._ui_callback = ui_callback
        self._fh = log_path.open("w", encoding="utf-8", newline="\n")

    def close(self) -> None:
        try:
            self._fh.close()
        except Exception:
            pass

    def line(self, text: str) -> None:
        line = text.rstrip("\n")
        print(line)
        self._fh.write(line + "\n")
        self._fh.flush()
        if self._ui_callback:
            self._ui_callback(line)


def _quote_arg(value: str) -> str:
    if not value:
        return '""'
    if any(ch.isspace() for ch in value) or '"' in value:
        return '"' + value.replace('"', '\\"') + '"'
    return value


def _try_import_sots_backup():
    python_root = Path(__file__).resolve().parents[2]
    sots_backup_dir = python_root / "sots_backup"
    if not sots_backup_dir.is_dir():
        return None
    if str(sots_backup_dir) not in sys.path:
        sys.path.insert(0, str(sots_backup_dir))
    try:
        import sots_backup  # type: ignore

        return sots_backup
    except Exception:
        return None


def _norm_path(p: Path) -> str:
    return str(p).replace("/", "\\").lower().rstrip("\\")


def _is_excluded_windows_path(p: Path) -> bool:
    # Exclude common Windows/system/junction locations that are slow or permission-heavy.
    n = _norm_path(p)
    excluded_prefixes = [
        "c:\\windows",
        "c:\\program files\\windowsapps",
        "c:\\system volume information",
        "c:\\$recycle.bin",
        "c:\\recovery",
        "c:\\documents and settings",
        "c:\\perflogs",
    ]
    return any(n.startswith(prefix) for prefix in excluded_prefixes)


def scan_for_restic_exe_on_c_drive(
    logger: ActionLogger,
    *,
    max_seconds: float = 120.0,
    max_dirs: int = 250_000,
) -> str | None:
    """Best-effort scan for restic.exe on C:\ with a conservative exclusion list.

    This is intended as a one-time bootstrap to reduce PATH/winget friction.
    """
    start_time = time.time()

    # Prioritize likely locations first, but keep the search scoped to C:\.
    seeds = [
        Path("C:/UE5"),
        Path("C:/Tools"),
        Path("C:/Program Files"),
        Path("C:/Program Files (x86)"),
        Path("C:/"),
    ]

    queue: list[Path] = []
    for s in seeds:
        try:
            if s.exists():
                queue.append(s)
        except Exception:
            continue

    best: Path | None = None
    best_mtime = -1.0
    scanned_dirs = 0

    logger.line(f"[restic] Scanning C:\\ for restic.exe (max_seconds={max_seconds:g})")

    while queue:
        if (time.time() - start_time) > max_seconds:
            logger.line("[restic] Scan timed out; using best match if any.")
            break
        if scanned_dirs >= max_dirs:
            logger.line("[restic] Scan hit directory cap; using best match if any.")
            break

        current = queue.pop(0)
        scanned_dirs += 1
        if scanned_dirs % 2000 == 0:
            logger.line(f"[restic] scan progress: dirs={scanned_dirs}")

        try:
            if _is_excluded_windows_path(current):
                continue

            with os.scandir(current) as it:
                for entry in it:
                    try:
                        if entry.is_symlink():
                            continue
                        if entry.is_file(follow_symlinks=False) and entry.name.lower() == "restic.exe":
                            p = Path(entry.path)
                            try:
                                mtime = p.stat().st_mtime
                            except Exception:
                                mtime = -1.0
                            if mtime > best_mtime:
                                best = p
                                best_mtime = mtime
                            continue
                        if entry.is_dir(follow_symlinks=False):
                            child = Path(entry.path)
                            # Skip excluded and very common problematic roots early.
                            if _is_excluded_windows_path(child):
                                continue
                            queue.append(child)
                    except PermissionError:
                        continue
                    except OSError:
                        continue
        except PermissionError:
            continue
        except FileNotFoundError:
            continue
        except OSError:
            continue

    if best:
        logger.line(f"[restic] Found restic.exe: {best}")
        return str(best)
    logger.line("[restic] restic.exe not found during scan.")
    return None


class ResticClient:
    def __init__(
        self,
        *,
        restic_repo: Path,
        passfile: Path,
        log_dir: Path,
        restic_exe: str | None = None,
    ) -> None:
        self.restic_repo = restic_repo
        self.passfile = passfile
        self.log_dir = log_dir
        self.restic_exe = restic_exe
        self._restic_resolved: str | None = None

    def _resolve_restic_exe(self, logger: ActionLogger) -> str | None:
        if self._restic_resolved:
            return self._restic_resolved

        if self.restic_exe:
            if Path(self.restic_exe).is_file():
                self._restic_resolved = self.restic_exe
                return self._restic_resolved
            if shutil.which(self.restic_exe):
                self._restic_resolved = self.restic_exe
                return self._restic_resolved
            logger.line(f"[restic] restic_exe not found: {self.restic_exe}")

        # Portable restic.exe drop locations (useful when winget isn't available).
        # Default SOTS backup root is where the passfile lives.
        backup_root = self.passfile.expanduser().resolve().parent
        for candidate in [
            backup_root / "restic.exe",
            backup_root / "bin" / "restic.exe",
            backup_root / "restic" / "restic.exe",
            backup_root / "tools" / "restic.exe",
        ]:
            try:
                if candidate.is_file():
                    logger.line(f"[restic] Using portable exe: {candidate}")
                    self._restic_resolved = str(candidate)
                    return self._restic_resolved
            except Exception:
                continue

        sots_backup = _try_import_sots_backup()
        if sots_backup and hasattr(sots_backup, "_find_restic_exe"):
            found = sots_backup._find_restic_exe(logger)
            if found:
                self._restic_resolved = found
                return self._restic_resolved

        found = shutil.which("restic")
        if found:
            self._restic_resolved = found
            return self._restic_resolved

        return None

    def try_resolve_restic_exe(self, logger: ActionLogger) -> str | None:
        return self._resolve_restic_exe(logger)

    def _env(self) -> dict[str, str]:
        env = os.environ.copy()
        env["RESTIC_PASSWORD_FILE"] = str(self.passfile)
        return env

    def _base_args(self, logger: ActionLogger) -> list[str]:
        restic_exe = self._resolve_restic_exe(logger)
        if not restic_exe:
            raise FileNotFoundError(
                "restic not found. Install restic and retry, or set 'restic_exe' in "
                "DevTools/python/sots_backup_gui/gui_config.json to a full path (e.g. C:/UE5/SOTS_Backup/restic.exe)."
            )
        return [restic_exe, "-r", str(self.restic_repo)]

    def check_requirements(self, logger: ActionLogger) -> None:
        logger.line("[check] Validating restic + passfile + repo")
        restic_exe = self._resolve_restic_exe(logger)
        if not restic_exe:
            raise FileNotFoundError(
                "restic not found. Install it (e.g. winget install restic.restic), or set 'restic_exe' in "
                "DevTools/python/sots_backup_gui/gui_config.json to a full path. You can also place a portable restic.exe next to the passfile (e.g. C:/UE5/SOTS_Backup/restic.exe)."
            )
        if not self.passfile.is_file():
            raise FileNotFoundError(
                f"Restic passfile missing: {self.passfile} (see Docs/SOTS_Backup_Restic.md)."
            )
        if not self.restic_repo.exists():
            raise FileNotFoundError(f"Restic repo not found: {self.restic_repo}")

        rc, _ = self.run_capture([restic_exe, "version"], logger=logger)
        if rc != 0:
            raise RuntimeError("restic version check failed.")

    def run_capture(self, args: list[str], *, logger: ActionLogger) -> tuple[int, str]:
        cmd_str = " ".join([_quote_arg(a) for a in args])
        logger.line(f"[cmd] {cmd_str}")
        proc = subprocess.run(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            env=self._env(),
        )
        out = proc.stdout or ""
        for line in out.splitlines():
            logger.line(line)
        logger.line(f"[cmd] exit={proc.returncode}")
        return proc.returncode, out

    def run_stream(self, args: list[str], *, logger: ActionLogger) -> int:
        cmd_str = " ".join([_quote_arg(a) for a in args])
        logger.line(f"[cmd] {cmd_str}")
        proc = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            env=self._env(),
        )
        assert proc.stdout is not None
        for line in proc.stdout:
            logger.line(line.rstrip("\n"))
        rc = proc.wait()
        logger.line(f"[cmd] exit={rc}")
        return rc

    def list_snapshots(self, logger: ActionLogger) -> list[dict]:
        args = self._base_args(logger) + ["snapshots", "--json"]
        rc, out = self.run_capture(args, logger=logger)
        if rc != 0:
            raise RuntimeError("restic snapshots failed.")
        data = self._parse_json(out, logger)
        normalized = [self._normalize_snapshot(item) for item in data]
        normalized.sort(key=lambda s: _parse_time_key(s.get("time")), reverse=True)
        return normalized

    def restore_snapshot(self, snapshot_id: str, target: Path, *, logger: ActionLogger) -> None:
        args = self._base_args(logger) + ["restore", snapshot_id, "--target", str(target)]
        rc = self.run_stream(args, logger=logger)
        if rc != 0:
            raise RuntimeError("restic restore failed.")

    def forget_snapshot(self, snapshot_id: str, *, prune: bool, logger: ActionLogger) -> None:
        args = self._base_args(logger) + ["forget", snapshot_id]
        if prune:
            args.append("--prune")
        rc = self.run_stream(args, logger=logger)
        if rc != 0:
            raise RuntimeError("restic forget failed.")

    def prune_repo(self, logger: ActionLogger) -> None:
        args = self._base_args(logger) + ["prune"]
        rc = self.run_stream(args, logger=logger)
        if rc != 0:
            raise RuntimeError("restic prune failed.")

    def _parse_json(self, text: str, logger: ActionLogger) -> list[dict]:
        try:
            return json.loads(text)
        except Exception as exc:
            logger.line(f"[warn] snapshots JSON parse failed: {exc}")
            start = text.find("[")
            end = text.rfind("]")
            if start >= 0 and end > start:
                try:
                    return json.loads(text[start : end + 1])
                except Exception as exc2:
                    logger.line(f"[error] JSON recovery failed: {exc2}")
            raise

    def _normalize_snapshot(self, item: dict) -> dict:
        snapshot_id = str(item.get("id", ""))
        short_id = str(item.get("short_id") or snapshot_id[:8])
        tags = item.get("tags") or []
        if not isinstance(tags, list):
            tags = [str(tags)]
        paths = item.get("paths") or []
        if not isinstance(paths, list):
            paths = [str(paths)]
        tag_list = [str(t) for t in tags]
        tag_note_title = _extract_note_title_tag(tag_list)
        tag_note = _extract_note_tag(tag_list)
        return {
            "id": snapshot_id,
            "short_id": short_id,
            "time": str(item.get("time") or ""),
            "tags": [str(t) for t in tags],
            "hostname": str(item.get("hostname") or item.get("host") or ""),
            "username": str(item.get("username") or item.get("user") or ""),
            "paths": [str(p) for p in paths],
            "tag_note": tag_note,
            "tag_note_title": tag_note_title,
        }

    def update_note_tags(
        self,
        snapshot_id: str,
        note_title: str,
        note_body: str,
        existing_tags: list[str],
        logger: ActionLogger,
    ) -> None:
        add_tags: list[str] = []
        title_clean = _sanitize_tag(note_title, max_len=48)
        body_clean = _sanitize_tag(note_body, max_len=48)
        if note_title.strip():
            add_tags.append(f"note_title_{title_clean}")
        if note_body.strip():
            add_tags.append(f"note_{body_clean}")

        tags_to_remove = [t for t in existing_tags if t.startswith("note_")]
        tags_to_remove = [t for t in tags_to_remove if t not in add_tags]

        if not add_tags and not tags_to_remove:
            logger.line("[tag] No note tag changes needed.")
            return

        args = self._base_args(logger) + ["tag"]
        for tag in add_tags:
            args += ["--add", tag]
        for tag in tags_to_remove:
            args += ["--remove", tag]
        args.append(snapshot_id)

        logger.line(f"[tag] Updating note tags for {snapshot_id}")
        rc = self.run_stream(args, logger=logger)
        if rc != 0:
            raise RuntimeError("restic tag update failed.")


def _parse_time_key(value: str | None) -> str:
    return value or ""


def _extract_note_tag(tags: list[str]) -> str:
    note_tags = [t for t in tags if t.startswith("note_") and not t.startswith("note_title_")]
    if not note_tags:
        return ""
    tag = note_tags[-1]
    return tag[len("note_") :]


def _extract_note_title_tag(tags: list[str]) -> str:
    note_tags = [t for t in tags if t.startswith("note_title_")]
    if not note_tags:
        return ""
    tag = note_tags[-1]
    return tag[len("note_title_") :]


def _sanitize_tag(value: str, *, max_len: int = 48) -> str:
    cleaned = []
    for ch in value.strip():
        if ch.isalnum() or ch in "._-":
            cleaned.append(ch)
        else:
            cleaned.append("_")
    out = "".join(cleaned).strip("_.-")
    if not out:
        return "x"
    return out[:max_len]
