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


def iter_files(exts_pattern: str | None):
    ext_re = re.compile(exts_pattern) if exts_pattern else None
    for path in PROJECT_ROOT.rglob("*"):
        if not path.is_file():
            continue
        if ext_re is not None and not ext_re.search(path.suffix):
            continue
        yield path


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="SOTS DevTools: include map (reverse)")
    ap.add_argument("--target", required=True, help="Filename or substring in include path")
    ap.add_argument("--exts", default=r"\\.h|\\.cpp")
    ap.add_argument("--mode", choices=["reverse"], default="reverse")
    args = ap.parse_args(argv)

    ts = now_tag()
    log_path = LOG_DIR / f"include_map_{ts}.log"

    target = args.target
    inc_re = re.compile(r'#\\s*include\\s*[\\"<](.*)[\\">]')

    total_hits = 0
    with log_path.open("w", encoding="utf-8") as log:
        print(f"[INCLUDE_MAP] mode={args.mode} target={target!r} exts={args.exts}", file=log)

        for path in iter_files(args.exts):
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue

            for lineno, line in enumerate(text.splitlines(), 1):
                m = inc_re.search(line)
                if not m:
                    continue
                inc_path = m.group(1)
                if target in inc_path:
                    total_hits += 1
                    print(f"{path}:{lineno}: {line.rstrip()}", file=log)

        print(f"\\n[SUMMARY] total_hits={total_hits}", file=log)

    print(f"[INCLUDE_MAP] Done. Hits={total_hits}. Log: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
