from __future__ import annotations
import argparse
import json
import re
from pathlib import Path
from typing import List, Dict

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def apply_edits(root: Path, config: Dict, dry_run: bool) -> int:
    exts = set(config.get("exts", [".h", ".cpp", ".cs", ".ini", ".uplugin"]))
    patterns = config.get("patterns", [])
    if not patterns:
        print("[WARN] No patterns defined in config.")
        return 0

    files_changed = 0

    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in exts:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        original = text
        for p in patterns:
            mode = p.get("mode", "regex")
            find = p.get("find", "")
            repl = p.get("replace", "")
            if not find:
                continue
            if mode == "regex":
                text = re.sub(find, repl, text)
            else:
                text = text.replace(find, repl)
        if text != original:
            files_changed += 1
            if dry_run:
                print(f"[DRY-RUN] Would change: {path}")
            else:
                print(f"[WRITE] {path}")
                path.write_text(text, encoding="utf-8")

    return files_changed


def main() -> None:
    parser = argparse.ArgumentParser(description="Mass regex/literal edit based on JSON config.")
    parser.add_argument("config", type=str, help="Path to JSON config file.")
    parser.add_argument("--root", type=str, default=".", help="Root folder (default: project root).")
    parser.add_argument("--dry-run", action="store_true", help="Only print what would be changed.")
    args = parser.parse_args()

    confirm_start("mass_regex_edit")

    cfg_path = Path(args.config).resolve()
    if not cfg_path.exists():
        print(f"[ERROR] Config file not found: {cfg_path}")
        print_llm_summary("mass_regex_edit", CONFIG=str(cfg_path), FILES_CHANGED=0, ERROR="CONFIG_NOT_FOUND")
        confirm_exit()
        return

    config = json.loads(cfg_path.read_text(encoding="utf-8"))

    project_root = get_project_root()
    root = Path(args.root)
    if not root.is_absolute():
        root = project_root / root

    print(f"[INFO] Root: {root}")
    print(f"[INFO] Using config: {cfg_path}")

    files_changed = apply_edits(root, config, args.dry_run)
    print(f"[SUMMARY] Files changed: {files_changed} (dry-run={args.dry_run})")

    print_llm_summary(
        "mass_regex_edit",
        CONFIG=str(cfg_path),
        ROOT=str(root),
        FILES_CHANGED=files_changed,
        DRY_RUN=args.dry_run,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
