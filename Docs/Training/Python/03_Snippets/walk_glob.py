from __future__ import annotations

from pathlib import Path
from typing import Iterable

DEFAULT_EXCLUDES = {".git", "Binaries", "Intermediate", "Saved", ".vs", ".idea", "__pycache__"}

def iter_files(root: Path, *, pattern: str = "*", excludes: set[str] | None = None) -> Iterable[Path]:
    ex = excludes or DEFAULT_EXCLUDES
    for p in root.rglob(pattern):
        if any(part in ex for part in p.parts):
            continue
        if p.is_file():
            yield p
