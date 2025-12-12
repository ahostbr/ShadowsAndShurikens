from __future__ import annotations
from pathlib import Path
import json
import shutil


def remove_dir(path: Path, dry_run: bool = True) -> None:
    if not path.exists():
        return
    if dry_run:
        print(f"[DRY-RUN] Would remove: {path}")
    else:
        print(f"[REMOVE] {path}")
        shutil.rmtree(path, ignore_errors=True)


def load_json(path: Path) -> dict:
    """Load a JSON file, returning {} on failure."""
    try:
        text = path.read_text(encoding="utf-8")
    except FileNotFoundError:
        return {}
    except Exception as exc:
        print(f"[ERROR] Failed to read JSON {path}: {exc}")
        return {}
    try:
        return json.loads(text)
    except Exception as exc:
        print(f"[ERROR] Failed to parse JSON {path}: {exc}")
        return {}


def edit_json_file(path: Path, editor, dry_run: bool = False) -> bool:
    """Apply an editor callback to a JSON document on disk.

    The editor is called as ``editor(data: dict) -> bool`` and should return True
    if it mutated the data. When ``dry_run`` is True, no file is written.
    """
    data = load_json(path)
    changed = editor(data)
    if not changed:
        return False
    if dry_run:
        print(f"[DRY-RUN] Would write JSON to {path}")
        return True
    try:
        path.write_text(json.dumps(data, indent=2, sort_keys=True), encoding="utf-8")
        print(f"[WRITE] {path}")
        return True
    except Exception as exc:
        print(f"[ERROR] Failed to write JSON {path}: {exc}")
        return False
