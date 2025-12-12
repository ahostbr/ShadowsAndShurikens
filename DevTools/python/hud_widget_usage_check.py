"""Ensure HUD/notification subsystems are only manipulated from the UI module."""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SKIP_DIRS = {"Binaries", "DerivedDataCache", "Intermediate", ".git", "Saved"}
PATTERN_SPECS = (
    ("HUD SetHealth", re.compile(r"\bSetHealthPercent\b")),
    ("HUD SetDetection", re.compile(r"\bSetDetectionLevel\b")),
    ("HUD SetObjective", re.compile(r"\bSetObjectiveText\b")),
    ("Notification Push", re.compile(r"\bPushNotification\b")),
    ("HUD Get", re.compile(r"\bUSOTS_HUDSubsystem::Get\b")),
    ("Notification Get", re.compile(r"\bUSOTS_NotificationSubsystem::Get\b")),
)


def _is_allowed_file(path: Path) -> bool:
    try:
        rel = path.relative_to(ROOT)
    except ValueError:
        return False

    if len(rel.parts) < 2:
        return False

    return rel.parts[0] == "Plugins" and rel.parts[1] == "SOTS_UI"


def _should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def find_violations() -> list[tuple[Path, int, str, str]]:
    violations: list[tuple[Path, int, str, str]] = []

    for dirpath, _, filenames in os.walk(ROOT):
        dir_path = Path(dirpath)
        if _should_skip(dir_path):
            continue

        for filename in filenames:
            if not filename.endswith((".cpp", ".cc", ".h", ".hpp")):
                continue

            file_path = dir_path / filename
            if _should_skip(file_path):
                continue

            allowed = _is_allowed_file(file_path)

            try:
                with open(file_path, "r", encoding="utf-8", errors="ignore") as fh:
                    for lineno, line in enumerate(fh, start=1):
                        for name, pattern in PATTERN_SPECS:
                            if pattern.search(line) and not allowed:
                                violations.append((file_path, lineno, name, line.strip()))
            except OSError:
                continue

    return violations


def main() -> None:
    violations = find_violations()
    if violations:
        print("[ERROR] HUD/notification subsystem usage found outside Plugins/SOTS_UI")
        for path, line, label, code in violations:
            rel = path.relative_to(ROOT)
            print(f" - {rel}:{line} ({label}) -> {code}")
        sys.exit(1)

    print("[OK] HUD and notification subsystems only touched by Plugins/SOTS_UI.")


if __name__ == "__main__":
    main()