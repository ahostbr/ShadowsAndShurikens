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
    ap = argparse.ArgumentParser(description="SOTS DevTools: GameplayTag usage scan")
    ap.add_argument("--tag", required=True, help="Full or partial tag string")
    ap.add_argument("--exts", default=r"\\.h|\\.cpp|\\.ini")
    args = ap.parse_args(argv)

    ts = now_tag()
    log_path = LOG_DIR / f"tag_usage_scan_{ts}.log"

    tag = args.tag
    tag_re = re.compile(re.escape(tag))

    decl_hits = 0
    use_hits = 0

    with log_path.open("w", encoding="utf-8") as log:
        print(f"[TAG_USAGE] tag={tag!r} exts={args.exts}", file=log)

        print("\\n[DECLARATIONS]", file=log)
        for path in iter_files(args.exts):
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue

            for lineno, line in enumerate(text.splitlines(), 1):
                if tag_re.search(line):
                    if "TagManager" in str(path) or "Tags" in str(path):
                        decl_hits += 1
                        print(f"{path}:{lineno}: {line.rstrip()}", file=log)

        print("\\n[USAGES]", file=log)
        for path in iter_files(args.exts):
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue

            for lineno, line in enumerate(text.splitlines(), 1):
                if tag_re.search(line):
                    use_hits += 1
                    print(f"{path}:{lineno}: {line.rstrip()}", file=log)

        print(f"\\n[SUMMARY] decl_hits={decl_hits} use_hits={use_hits}", file=log)

    print(f"[TAG_USAGE] Done. decl_hits={decl_hits} use_hits={use_hits}. Log: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
