from __future__ import annotations

import json
from pathlib import Path
from typing import Set

from ..hub_config import REPORTS_DIR

PINS_PATH = REPORTS_DIR / "sots_hub_pins.json"


def load_pins(logger) -> Set[str]:
    if not PINS_PATH.exists():
        logger.info("[Hub] No pin file found; starting empty.")
        return set()
    try:
        pins = set(json.loads(PINS_PATH.read_text(encoding="utf-8")))
        logger.info("[Hub] Loaded %d pinned plugins", len(pins))
        return pins
    except Exception as exc:  # pragma: no cover - defensive
        logger.warning("[Hub] Failed to read pins file: %s", exc)
        return set()


def save_pins(pins: Set[str], logger) -> None:
    try:
        PINS_PATH.parent.mkdir(parents=True, exist_ok=True)
        PINS_PATH.write_text(json.dumps(sorted(pins)), encoding="utf-8")
        logger.info("[Hub] Saved %d pinned plugins -> %s", len(pins), PINS_PATH)
    except Exception as exc:  # pragma: no cover - defensive
        logger.warning("[Hub] Failed to persist pins: %s", exc)

