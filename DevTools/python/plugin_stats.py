from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent
PLUGINS_DIR = PROJECT_ROOT / "Plugins"
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)


def now_tag() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def iter_plugin_files(plugin_root: Path, exts_pattern: str | None):
    ext_re = re.compile(exts_pattern) if exts_pattern else None
    for path in plugin_root.rglob("*"):
        if not path.is_file():
            continue
        if ext_re is not None and not ext_re.search(path.suffix):
            continue
        yield path


def count_loc(path: Path) -> int:
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return 0
    return sum(1 for line in text.splitlines() if line.strip())


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="SOTS DevTools: plugin size stats")
    ap.add_argument("--plugins", required=True, help="Names separated by ';' or ','")
    ap.add_argument("--exts", default=r"\\.h|\\.cpp|\\.inl")
    args = ap.parse_args(argv)

    plugin_names = [p.strip() for p in re.split(r"[;,]", args.plugins) if p.strip()]

    ts = now_tag()
    log_path = LOG_DIR / f"plugin_stats_{ts}.log"

    with log_path.open("w", encoding="utf-8") as log:
        print(f"[PLUGIN_STATS] plugins={plugin_names} exts={args.exts}", file=log)

        for name in plugin_names:
            plugin_root = PLUGINS_DIR / name
            if not plugin_root.is_dir():
                print(f"\\n[PLUGIN] {name} (MISSING at {plugin_root})", file=log)
                continue

            print(f"\\n[PLUGIN] {name}", file=log)

            ext_counts = Counter()
            file_locs = []

            for path in iter_plugin_files(plugin_root, args.exts):
                loc = count_loc(path)
                ext_counts[path.suffix] += loc
                file_locs.append((loc, path))

            total_loc = sum(loc for loc, _ in file_locs)
            print(f"  Total LOC: {total_loc}", file=log)
            for ext, loc in ext_counts.items():
                print(f"  {ext or '<noext>'}: {loc} LOC", file=log)

            file_locs.sort(reverse=True)
            top = file_locs[:5]
            print("  Top files:", file=log)
            for loc, path in top:
                print(f"    {loc:6d}  {path}", file=log)

    print(f"[PLUGIN_STATS] Done. Log: {log_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
