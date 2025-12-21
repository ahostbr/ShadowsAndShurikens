from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict

from PySide6 import QtCore

DEFAULTS = {
    "project_root": "",
    "video_control_path": r"E:\SAS\SOTS_VIDEO_CONTROL\bin\Debug\net9.0-windows\SOTSVideoControl.exe",
    "last_disk_root": "",
    "docs_watch_poll_seconds": 10,
    "ui/palette_recent": [],
    "reports/smart_enabled": True,
    "reports/stale_minutes": 30,
    "ui/enable_writes": False,
    "disk/auto_scan": True,
    "disk/auto_scan_depth": 6,
    "openai/usage_enabled": True,
    "openai/org": "org-ZPaoTmtYZJ0I5V69vAS2dPEq",
}


class Settings:
    def __init__(self, org: str = "SOTS", app: str = "StatsHub") -> None:
        self.qsettings = QtCore.QSettings(org, app)
        self.state_path = self._state_file()
        self.data: Dict[str, Any] = {}
        self._load()

    @staticmethod
    def _state_file() -> Path:
        here = Path(__file__).resolve()
        return here.parents[1] / "_state" / "sots_stats_hub_settings.json"

    def _load(self) -> None:
        for k, v in DEFAULTS.items():
            try:
                val = self.qsettings.value(k, v)
            except Exception:
                val = v
            self.data[k] = val if val not in (None, "") else v

        try:
            if self.state_path.exists():
                obj = json.loads(self.state_path.read_text(encoding="utf-8"))
                if isinstance(obj, dict):
                    self.data.update({k: obj.get(k, self.data.get(k)) for k in DEFAULTS})
        except Exception:
            pass

        for k, v in DEFAULTS.items():
            self.data.setdefault(k, v)

    def save(self) -> None:
        try:
            for k, v in self.data.items():
                self.qsettings.setValue(k, v)
        except Exception:
            pass

        try:
            self.state_path.parent.mkdir(parents=True, exist_ok=True)
            self.state_path.write_text(json.dumps(self.data, indent=2), encoding="utf-8")
        except Exception:
            pass

    def get(self, key: str, default: Any = None) -> Any:
        return self.data.get(key, default)

    def set(self, key: str, value: Any) -> None:
        self.data[key] = value
        self.save()
