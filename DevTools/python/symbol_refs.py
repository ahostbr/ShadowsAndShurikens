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


def classify_line(line: str, symbol: str) -> str:
    stripped = line.strip()
    if stripped.startswith("class ") and symbol in stripped:
        return "definition"
    if stripped.startswith("struct ") and symbol in stripped:
        return "definition"
    if "UCLASS" in stripped and symbol in stripped:
        return "definition"
    if "USTRUCT" in stripped and symbol in stripped:
        return "definition"
    if symbol in stripped and "(" in stripped and stripped.endswith(");"):
        return "declaration"
    return "usage"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="SOTS DevTools: symbol reference scan")
    ap.add_argument("--symbol", required=True)
    ap.add_argument("--exts", default=r"\\.h|\\.cpp")
    args = ap.parse_args(argv)

    ts = now_tag()
    log_path = LOG_DIR / f"symbol_refs_{ts}.log"

    symbol = args.symbol
    sym_re = re.compile(r"\\b" + re.escape(symbol) + r"\\b")

    defs = []
    decls = []
    uses = []

    with log_path.open("w", encoding="utf-8") as log:
        print(f"[SYMBOL_REFS] symbol={symbol!r} exts={args.exts}", file=log)

        for path in iter_files(args.exts):
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue

            for lineno, line in enumerate(text.splitlines(), 1):
                if not sym_re.search(line):
                    continue
                kind = classify_line(line, symbol)
                entry = f"{path}:{lineno}: {line.rstrip()}"
                if kind == "definition":
                    defs.append(entry)
                elif kind == "declaration":
                    decls.append(entry)
                else:
                    uses.append(entry)

        print("\\n[DEFINITIONS]", file=log)
        for e in defs:
            print(e, file=log)

        print("\\n[DECLARATIONS]", file=log)
        for e in decls:
            print(e, file=log)

        print("\\n[USAGES]", file=log)
        for e in uses:
            print(e, file=log)

        print(
            f"\\n[SUMMARY] definitions={len(defs)} declarations={len(decls)} usages={len(uses)}",
            file=log,
        )

    print(
        f"[SYMBOL_REFS] Done. defs={len(defs)} decls={len(decls)} uses={len(uses)}. "
        f"Log: {log_path}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
