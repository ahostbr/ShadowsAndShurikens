from __future__ import annotations

import hashlib
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


class DuplicateScanCancelled(Exception):
    """Raised when a duplicate scan is cancelled."""


@dataclass
class DuplicateScanOptions:
    skip_symlinks: bool = True
    min_size_bytes: int = 1 * 1024 * 1024
    quick_hash_bytes: int = 256 * 1024
    hash_mode: str = "quick_then_full"  # or "full_only"

    def to_dict(self) -> dict:
        return {
            "skip_symlinks": self.skip_symlinks,
            "min_size_bytes": self.min_size_bytes,
            "quick_hash_bytes": self.quick_hash_bytes,
            "hash_mode": self.hash_mode,
        }


@dataclass
class DuplicateScanStats:
    files_scanned: int = 0
    dirs_scanned: int = 0
    errors: int = 0
    candidate_buckets: int = 0
    quick_groups: int = 0
    dup_groups: int = 0
    dup_files: int = 0
    wasted_bytes: int = 0

    def to_dict(self) -> dict:
        return {
            "files_scanned": self.files_scanned,
            "dirs_scanned": self.dirs_scanned,
            "errors": self.errors,
            "candidate_buckets": self.candidate_buckets,
            "quick_groups": self.quick_groups,
            "dup_groups": self.dup_groups,
            "dup_files": self.dup_files,
            "wasted_bytes": self.wasted_bytes,
        }


ProgressCallback = Callable[[str, int, int], None]
CancelCallback = Callable[[], bool]


def _should_skip_symlink(entry: os.DirEntry, skip_symlinks: bool) -> bool:
    try:
        return skip_symlinks and entry.is_symlink()
    except Exception:
        return skip_symlinks


def _quick_hash(path: Path, quick_bytes: int) -> str:
    """Hash size + first/last quick_bytes (or full file if smaller)."""
    h = hashlib.sha256()
    size = path.stat().st_size
    h.update(str(size).encode("utf-8"))
    with path.open("rb") as fh:
        head = fh.read(quick_bytes)
        if size > quick_bytes:
            fh.seek(max(0, size - quick_bytes))
            tail = fh.read(quick_bytes)
        else:
            tail = b""
    h.update(head)
    h.update(tail)
    return h.hexdigest()


def _full_hash(path: Path, chunk_size: int = 1024 * 1024) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        while True:
            chunk = fh.read(chunk_size)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def scan_duplicates(
    root_path: str | Path,
    options: DuplicateScanOptions | None = None,
    progress_cb: ProgressCallback | None = None,
    cancel_cb: CancelCallback | None = None,
) -> tuple[list[dict], DuplicateScanStats]:
    opts = options or DuplicateScanOptions()
    root = Path(root_path)
    if not root.exists():
        raise FileNotFoundError(f"Path does not exist: {root}")

    stats = DuplicateScanStats()
    size_buckets: dict[int, list[str]] = {}

    def cancelled() -> bool:
        return cancel_cb is not None and cancel_cb()

    def emit_progress(current_path: str) -> None:
        if progress_cb:
            progress_cb(current_path, stats.files_scanned, len(size_buckets))

    def walk(current: Path) -> None:
        if cancelled():
            raise DuplicateScanCancelled()
        stats.dirs_scanned += 1
        try:
            with os.scandir(current) as it:
                entries = list(it)
        except Exception:
            stats.errors += 1
            emit_progress(str(current))
            return

        for entry in entries:
            if cancelled():
                raise DuplicateScanCancelled()
            try:
                if _should_skip_symlink(entry, opts.skip_symlinks):
                    continue
                if entry.is_dir(follow_symlinks=False):
                    walk(Path(entry.path))
                elif entry.is_file(follow_symlinks=False):
                    try:
                        size = int(entry.stat(follow_symlinks=False).st_size)
                    except Exception:
                        stats.errors += 1
                        continue
                    stats.files_scanned += 1
                    size_buckets.setdefault(size, []).append(entry.path)
                    if stats.files_scanned % 200 == 0:
                        emit_progress(entry.path)
            except DuplicateScanCancelled:
                raise
            except Exception:
                stats.errors += 1

    emit_progress(str(root))
    walk(root)

    candidates = {
        size: paths for size, paths in size_buckets.items() if len(paths) > 1 and size >= opts.min_size_bytes
    }
    stats.candidate_buckets = len(candidates)

    final_groups: list[dict] = []

    def add_groups(group_map: dict[str, list[str]], size: int) -> None:
        for hash_value, paths in group_map.items():
            if len(paths) > 1:
                final_groups.append({"size": size, "hash": hash_value, "paths": sorted(paths)})

    if opts.hash_mode == "quick_then_full":
        for size, paths in candidates.items():
            quick_buckets: dict[str, list[str]] = {}
            for p in paths:
                if cancelled():
                    raise DuplicateScanCancelled()
                try:
                    qh = _quick_hash(Path(p), opts.quick_hash_bytes)
                except Exception:
                    stats.errors += 1
                    continue
                quick_buckets.setdefault(qh, []).append(p)
            stats.quick_groups += sum(1 for v in quick_buckets.values() if len(v) > 1)

            full_buckets: dict[str, list[str]] = {}
            for qh, qpaths in quick_buckets.items():
                if len(qpaths) < 2:
                    continue
                for p in qpaths:
                    if cancelled():
                        raise DuplicateScanCancelled()
                    try:
                        fh = _full_hash(Path(p))
                    except Exception:
                        stats.errors += 1
                        continue
                    full_buckets.setdefault(fh, []).append(p)
            add_groups(full_buckets, size)
    else:  # full_only
        for size, paths in candidates.items():
            full_buckets: dict[str, list[str]] = {}
            for p in paths:
                if cancelled():
                    raise DuplicateScanCancelled()
                try:
                    fh = _full_hash(Path(p))
                except Exception:
                    stats.errors += 1
                    continue
                full_buckets.setdefault(fh, []).append(p)
            add_groups(full_buckets, size)

    final_groups.sort(key=lambda g: g["size"], reverse=True)
    stats.dup_groups = len(final_groups)
    stats.dup_files = sum(len(g["paths"]) for g in final_groups)
    stats.wasted_bytes = sum((len(g["paths"]) - 1) * g["size"] for g in final_groups)
    return final_groups, stats

