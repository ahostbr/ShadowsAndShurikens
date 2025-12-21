from __future__ import annotations

import os
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Iterable, Tuple

from .model import FileEntry, Node, ScanStats


ProgressCallback = Callable[[str], None]


class ScanCancelled(Exception):
    """Raised when a scan is cancelled mid-run."""


@dataclass
class ScanOptions:
    skip_symlinks: bool = True
    max_depth: int = 0  # 0 = unlimited
    top_limit: int = 200
    exclusions: list[str] = field(default_factory=list)
    quiet_mode: bool = False
    progress_interval: float = 0.75


def _push_top(heap: list[Tuple[int, str, FileEntry]], entry: FileEntry, limit: int) -> None:
    """Maintain a min-heap of the top N file sizes with stable tiebreaker."""
    import heapq

    heapq.heappush(heap, (entry.size, entry.path, entry))
    if len(heap) > limit:
        heapq.heappop(heap)


def _merge_top_lists(destination: list[Tuple[int, str, FileEntry]], source: Iterable[FileEntry], limit: int) -> None:
    for item in source:
        _push_top(destination, item, limit)


def scan_path(
    root_path: str | Path,
    options: ScanOptions | None = None,
    progress_cb: ProgressCallback | None = None,
    cancel_event: threading.Event | None = None,
) -> tuple[Node, ScanStats]:
    """
    Scan the filesystem starting at root_path and return the root Node + stats.

    - Uses os.scandir() for speed.
    - Skips symlinks by default.
    - Respects a cancel_event checked frequently.
    - Handles permission errors by logging and continuing.
    """
    opts = options or ScanOptions()
    root = Path(root_path)
    if not root.exists():
        raise FileNotFoundError(f"Path does not exist: {root}")

    stats = ScanStats()
    stats.started_at = time.time()
    normalized_exclusions = [p.lower() for p in (opts.exclusions or []) if p]
    progress_interval = max(0.1, float(opts.progress_interval))
    last_progress_emit = 0.0

    def emit(msg: str, *, force: bool = False, progress: bool = False) -> None:
        nonlocal last_progress_emit
        if progress_cb:
            now = time.time()
            if force:
                progress_cb(msg)
                last_progress_emit = now
                return
            if progress:
                interval = 0.15 if not opts.quiet_mode else progress_interval
                if now - last_progress_emit < interval:
                    return
                last_progress_emit = now
            progress_cb(msg)

    def log_progress(current: Path) -> None:
        emit(
            f"[PROGRESS] dirs={stats.total_dirs} files={stats.total_files} errors={stats.total_errors} path={current}",
            progress=True,
        )

    def is_excluded(path: Path) -> bool:
        if not normalized_exclusions:
            return False
        target = str(path).lower()
        segments = target.replace("\\", "/").split("/")
        for pat in normalized_exclusions:
            if pat in target or pat in segments:
                return True
        return False

    def should_cancel() -> bool:
        return cancel_event is not None and cancel_event.is_set()

    def walk(current: Path, depth: int) -> Node:
        if should_cancel():
            raise ScanCancelled()

        name = current.name or str(current)
        node = Node(name=name, path=str(current.resolve()), size=0)
        stats.total_dirs += 1
        if opts.quiet_mode:
            log_progress(current)
        elif stats.total_dirs == 1 or stats.total_dirs % 200 == 0:
            emit(f"[DIR] {current} (visited {stats.total_dirs})", progress=True)

        try:
            with os.scandir(current) as it:
                entries = list(it)
        except Exception as exc:  # noqa: BLE001
            stats.total_errors += 1
            emit(f"[ERROR] Could not read {current}: {exc}", force=True)
            return node

        top_heap: list[Tuple[int, str, FileEntry]] = []
        for entry in entries:
            if should_cancel():
                raise ScanCancelled()
            try:
                entry_path = Path(entry.path)
                if is_excluded(entry_path):
                    continue

                if opts.skip_symlinks and entry.is_symlink():
                    continue

                if entry.is_dir(follow_symlinks=False):
                    if opts.max_depth > 0 and depth >= opts.max_depth:
                        # Respect depth limit: do not descend further.
                        continue

                    child = walk(Path(entry.path), depth + 1)
                    child.parent = node
                    node.children.append(child)
                    node.size += child.size
                    _merge_top_lists(top_heap, child.top_files, opts.top_limit)
                elif entry.is_file(follow_symlinks=False):
                    try:
                        stat_result = entry.stat(follow_symlinks=False)
                    except Exception as exc:  # noqa: BLE001
                        stats.total_errors += 1
                        emit(f"[WARN] Could not stat file {entry.path}: {exc}", force=True)
                        continue

                    stats.total_files += 1
                    size = int(stat_result.st_size)
                    node.size += size
                    ext = Path(entry.name).suffix.lower()
                    file_entry = FileEntry(
                        name=entry.name,
                        path=str(Path(entry.path)),
                        size=size,
                        extension=ext,
                    )
                    _push_top(top_heap, file_entry, opts.top_limit)
            except ScanCancelled:
                raise
            except Exception as exc:  # noqa: BLE001
                stats.total_errors += 1
                emit(f"[WARN] Error handling {entry.path}: {exc}", force=True)

        node.top_files = [item[2] for item in sorted(top_heap, key=lambda t: (t[0], t[1]), reverse=True)]
        return node

    emit(
        f"[SCAN] Starting scan: {root} (quiet={opts.quiet_mode} depth={opts.max_depth} "
        f"skip_symlinks={opts.skip_symlinks} exclusions={len(normalized_exclusions)})",
        force=True,
    )
    root_node = walk(root, depth=0)
    stats.total_size = root_node.size
    stats.finished_at = time.time()
    elapsed = stats.finished_at - stats.started_at if stats.started_at else 0.0
    emit(
        f"[SCAN] Done: dirs={stats.total_dirs} files={stats.total_files} "
        f"errors={stats.total_errors} size={stats.total_size} elapsed={elapsed:.1f}s",
        force=True,
    )

    return root_node, stats
