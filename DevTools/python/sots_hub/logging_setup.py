from __future__ import annotations

import logging
import sys
from pathlib import Path


def setup_logging(log_path: Path) -> logging.Logger:
    """
    Configure a chatty logger that writes to both stdout and a local log file.
    The hub should never run silently; we always attach both handlers.
    """
    log_path.parent.mkdir(parents=True, exist_ok=True)

    logger = logging.getLogger("sots_hub")
    logger.setLevel(logging.INFO)
    logger.handlers.clear()

    fmt = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")

    stream_handler = logging.StreamHandler(stream=sys.stdout)
    stream_handler.setFormatter(fmt)
    logger.addHandler(stream_handler)

    file_handler = logging.FileHandler(log_path, encoding="utf-8")
    file_handler.setFormatter(fmt)
    logger.addHandler(file_handler)

    logger.info("SOTS Hub logging initialized -> %s", log_path)
    return logger
