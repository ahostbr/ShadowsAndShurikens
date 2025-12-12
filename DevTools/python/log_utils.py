from __future__ import annotations
from pathlib import Path
from datetime import datetime


def append_log(path: Path, message: str) -> None:
    try:
        with path.open("a", encoding="utf-8") as f:
            f.write(f"[{datetime.now().isoformat()}] {message}\n")
    except Exception as e:
        print(f"[WARN] Failed to write log: {e}")
