from __future__ import annotations

import os
from pathlib import Path


def _resolve_project_root() -> Path:
    env_root = os.environ.get("SOTS_PROJECT_ROOT", "").strip()
    if env_root:
        return Path(env_root).expanduser().resolve()

    here = Path(__file__).resolve()
    for candidate in here.parents:
        if (candidate / "DevTools").exists():
            return candidate
    # Fallback: assume DevTools/python/sots_hub layout
    return here.parent.parent.parent


PROJECT_ROOT = _resolve_project_root()
DEVTOOLS_DIR = PROJECT_ROOT / "DevTools"
PYTHON_DIR = DEVTOOLS_DIR / "python"
HUB_DIR = PYTHON_DIR / "sots_hub"
LOGS_DIR = DEVTOOLS_DIR / "logs"
REPORTS_DIR = DEVTOOLS_DIR / "reports"
