from __future__ import annotations

import logging
import sys
from datetime import datetime
from pathlib import Path

from ..hub_config import LOGS_DIR


def setup_logger() -> tuple[logging.Logger, Path]:
    LOGS_DIR.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = LOGS_DIR / f"sots_hub_{timestamp}.log"

    logger = logging.getLogger("sots_hub")
    logger.setLevel(logging.INFO)
    logger.handlers.clear()

    formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")

    stream_handler = logging.StreamHandler(stream=sys.stdout)
    stream_handler.setFormatter(formatter)
    logger.addHandler(stream_handler)

    file_handler = logging.FileHandler(log_path, encoding="utf-8")
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)

    print(f"[sots_hub] Log file: {log_path}")
    logger.info("SOTS Hub logging started. Log file: %s", log_path)
    return logger, log_path
