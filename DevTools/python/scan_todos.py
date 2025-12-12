from __future__ import annotations
import argparse
from pathlib import Path
from typing import List

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


DEFAULT_MARKERS = ["TODO", "FIXME"]


def scan_for_markers(root: Path, markers: List[str]) -> dict[Path, List[str]]:
    results: dict[Path, List[str]] = {}
    exts = {".h", ".cpp", ".cs", ".py"}

    for p in root.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in exts:
            continue
        try:
            text = p.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue
        lines = text.splitlines()
        hits: List[str] = []
        for idx, line in enumerate(lines, start=1):
            for m in markers:
                if m in line:
                    hits.append(f"L{idx}: {line.strip()}")
                    break
        if hits:
            results[p] = hits

    return results


def main() -> None:
    parser = argparse.ArgumentParser(description="Scan source files for TODO/FIXME markers.")
    parser.add_argument("--root", type=str, default=".", help="Root folder to scan (default: project root).")
    parser.add_argument("--markers", type=str, nargs="*", default=None, help="Markers to search for (default: TODO FIXME).")
    args = parser.parse_args()

    confirm_start("scan_todos")

    project_root = get_project_root()
    root = Path(args.root)
    if not root.is_absolute():
        root = project_root / root

    markers = args.markers if args.markers else DEFAULT_MARKERS

    print(f"[INFO] Scanning for markers {markers} under: {root}")

    results = scan_for_markers(root, markers)

    total_hits = sum(len(v) for v in results.values())

    if not results:
        print("[INFO] No markers found.")
    else:
        print("\n[MARKERS FOUND]")
        for path, hits in results.items():
            print(f"  {path}:")
            for h in hits[:20]:
                print(f"    {h}")
            if len(hits) > 20:
                print(f"    ... ({len(hits) - 20} more lines)")

    print("\n[SUMMARY]")
    print(f"  Files with markers: {len(results)}")
    print(f"  Total marker lines: {total_hits}")

    print_llm_summary(
        "scan_todos",
        ROOT=str(root),
        FILES_WITH_MARKERS=len(results),
        TOTAL_MARKERS=total_hits,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
