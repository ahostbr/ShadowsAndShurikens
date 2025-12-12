from __future__ import annotations
import argparse
from pathlib import Path
from typing import Dict, List

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def scan_for_fsots_structs(root: Path) -> Dict[str, List[Path]]:
    mapping: Dict[str, List[Path]] = {}
    for path in root.rglob("*.h"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        for line in text.splitlines():
            line = line.strip()
            if line.startswith("USTRUCT(") or line.startswith("USTRUCT(BlueprintType)"):
                # crude heuristic: look ahead for 'struct XXX'
                # we just search in the next few lines
                pass
        # Simpler: search by 'struct FSOTS_'
        for line in text.splitlines():
            if "struct FSOTS_" in line:
                parts = line.replace("{", " ").split()
                if "struct" in parts:
                    idx = parts.index("struct")
                    if idx + 1 < len(parts):
                        name = parts[idx + 1]
                        mapping.setdefault(name, []).append(path)
    return mapping


def main() -> None:
    parser = argparse.ArgumentParser(description="Scan for FSOTS_* USTRUCT declarations.")
    parser.add_argument("--root", type=str, default=".", help="Root folder (default: project root).")
    args = parser.parse_args()

    confirm_start("scan_fsots_structs")

    project_root = get_project_root()
    root = Path(args.root)
    if not root.is_absolute():
        root = project_root / root

    mapping = scan_for_fsots_structs(root)

    print("[FSOTS STRUCTS]")
    for name, headers in mapping.items():
        print(f"{name}:")
        for h in headers:
            print(f"  - {h}")

    dupes = {k: v for k, v in mapping.items() if len(v) > 1}
    if dupes:
        print("\n[DUPLICATES]")
        for name, headers in dupes.items():
            print(f"{name}: {len(headers)} headers")

    print_llm_summary(
        "scan_fsots_structs",
        ROOT=str(root),
        STRUCT_COUNT=len(mapping),
        DUPLICATE_COUNT=len(dupes),
    )

    confirm_exit()


if __name__ == "__main__":
    main()
