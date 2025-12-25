from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

def write_text_atomic(path: Path, data: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + f".tmp.{os.getpid()}")
    tmp.write_text(data, encoding="utf-8")
    tmp.replace(path)

def write_json_atomic(path: Path, obj: Any, *, indent: int = 2) -> None:
    write_text_atomic(path, json.dumps(obj, indent=indent, ensure_ascii=False))

def read_json(path: Path) -> Any | None:
    if not path.exists():
        return None
    raw = path.read_text(encoding="utf-8").strip()
    if not raw:
        return None
    return json.loads(raw)
