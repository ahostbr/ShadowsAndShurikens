from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

@dataclass(frozen=True)
class LogEvent:
    ts: str
    level: str
    msg: str
    data: dict[str, Any]

def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()

def log_jsonl(path: Path, level: str, msg: str, **data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    ev = LogEvent(ts=now_iso(), level=level, msg=msg, data=data)
    with path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(asdict(ev), ensure_ascii=False) + "\n")
