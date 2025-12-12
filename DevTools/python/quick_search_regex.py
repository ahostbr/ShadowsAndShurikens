from __future__ import annotations

import argparse
import re
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)


def now_tag() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def build_flags(flag_str: str) -> int:
    flags = 0
    for ch in (flag_str or ""):
        ch = ch.upper()
        if ch == "I":
            flags |= re.IGNORECASE
        elif ch == "M":
            flags |= re.MULTILINE
        elif ch == "S":
            flags |= re.DOTALL
    return flags


def iter_files(exts_pattern: str):
    ext_re = re.compile(exts_pattern) if exts_pattern else None
    for path in PROJECT_ROOT.rglob("*"):
        if not path.is_file():
            continue
        if ext_re is not None and not ext_re.search(path.suffix):
            continue
        yield path


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="SOTS DevTools: regex search with context")
    ap.add_argument("--pattern", required=True)
    ap.add_argument("--exts", required=True, help=r"Regex for file suffix, e.g. '\\.h|\\.cpp'")
    ap.add_argument("--flags", default="", help="Combination of I, M, S")
    ap.add_argument("--context", type=int, default=0, help="Context lines around each match")
    args = ap.parse_args(argv)

    ts = now_tag()
    log_path = LOG_DIR / f"quick_search_regex_{ts}.log"

    flags = build_flags(args.flags)
    try:
        rx = re.compile(args.pattern, flags)
    except re.error as e:
        print(f"[QUICK_SEARCH_REGEX] Invalid pattern {args.pattern!r}: {e}")
        return 1

    total_hits = 0
    with log_path.open("w", encoding="utf-8") as log:
        print(
            f"[QUICK_SEARCH_REGEX] pattern={args.pattern!r} exts={args.exts} "
            f"flags={args.flags!r} context={args.context}",
            file=log,
        )

        for path in iter_files(args.exts):
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except Exception as e:
                print(f"[SKIP] {path} ({e})", file=log)
                continue

            lines = text.splitlines()
            for idx, line in enumerate(lines):
                if not rx.search(line):
                    continue

                total_hits += 1
                start = max(0, idx - args.context)
                end = min(len(lines), idx + args.context + 1)

                print(f"\\n[HIT] {path}:{idx+1}", file=log)
                for cidx in range(start, end):
                    prefix = ">" if cidx == idx else " "
                    print(f"{prefix}{cidx+1:5d} {lines[cidx]}", file=log)

        print(f"\\n[SUMMARY] total_hits={total_hits}", file=log)

    print(f"[QUICK_SEARCH_REGEX] Done. Hits={total_hits}. Log: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
