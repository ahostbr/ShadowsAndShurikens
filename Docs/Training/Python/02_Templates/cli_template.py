#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

def eprint(msg: str) -> None:
    print(msg, file=sys.stderr)

def write_text_atomic(path: Path, data: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + f".tmp.{os.getpid()}")
    tmp.write_text(data, encoding="utf-8")
    tmp.replace(path)

def write_json_atomic(path: Path, obj: Any) -> None:
    write_text_atomic(path, json.dumps(obj, indent=2, ensure_ascii=False))

@dataclass(frozen=True)
class PlanStep:
    op: str
    src: str | None = None
    dst: str | None = None
    note: str | None = None

def build_plan(root: Path) -> list[PlanStep]:
    # TODO: replace with real plan
    return [PlanStep(op="Example", src=str(root), note="Replace with real ops")]

def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="Root path")
    ap.add_argument("--dry-run", action="store_true", help="Print plan only")
    ap.add_argument("--out-json", type=str, default="", help="Optional JSON output path")
    args = ap.parse_args(argv)

    root = Path(args.root).expanduser()
    if not root.exists():
        raise FileNotFoundError(f"Root does not exist: {root}")
    root = root.resolve()

    plan = build_plan(root)

    if args.dry_run:
        print(json.dumps([step.__dict__ for step in plan], indent=2, ensure_ascii=False))
        return 0

    # EXECUTE PLAN (example noop)
    for _step in plan:
        # TODO: execute steps
        pass

    if args.out_json:
        out_path = Path(args.out_json).expanduser().resolve()
        write_json_atomic(out_path, {"ok": True, "steps": [s.__dict__ for s in plan]})

    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except Exception as ex:
        eprint(f"ERROR: {ex}")
        raise SystemExit(1)
