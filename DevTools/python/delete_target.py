#!/usr/bin/env python3
"""
delete_target.py

Safe-ish deletion tool for SOTS DevTools.

- Deletes one or more files/folders passed via arguments.
- Refuses to delete anything outside the project root.
- Asks for confirmation unless --yes is supplied.
- Logs via llm_log so it never runs silently.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil

from project_paths import get_project_root
from llm_log import print_llm_summary


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Delete one or more files/folders under the SOTS project root."
    )
    parser.add_argument(
        "targets",
        nargs="+",
        help="Paths to delete (files or directories). Must live under the project root.",
    )
    parser.add_argument(
        "--yes",
        action="store_true",
        help="Skip interactive confirmation (use with care).",
    )

    args = parser.parse_args()

    project_root = get_project_root().resolve()
    print(f"[INFO] Project root: {project_root}")

    # Resolve and filter targets
    resolved_targets: list[Path] = []
    skipped_outside: list[str] = []

    for raw in args.targets:
        p = Path(raw).expanduser()
        if not p.is_absolute():
            p = (project_root / p).resolve()
        else:
            p = p.resolve()

        try:
            p.relative_to(project_root)
        except ValueError:
            print(f"[WARN] Skipping (outside project root): {p}")
            skipped_outside.append(str(p))
            continue

        resolved_targets.append(p)

    if not resolved_targets:
        print("[INFO] No valid targets under project root; nothing to delete.")
        print_llm_summary(
            "delete_target",
            status="no_valid_targets",
            PROJECT_ROOT=str(project_root),
            TARGETS=[],  # none
            SKIPPED_OUTSIDE=skipped_outside,
        )
        # no interactive exit prompt for automation
        return

    print("\nTargets to delete:")
    for p in resolved_targets:
        print(f"  - {p}")

    if not args.yes:
        answer = input("\nType DELETE to confirm deletion of these paths: ").strip()
        if answer != "DELETE":
            print("[INFO] Deletion aborted by user.")
            print_llm_summary(
                "delete_target",
                status="aborted",
                PROJECT_ROOT=str(project_root),
                TARGETS=[str(p) for p in resolved_targets],
                SKIPPED_OUTSIDE=skipped_outside,
            )
            # no interactive exit prompt for automation
            return

    deleted: list[str] = []
    missing: list[str] = []
    errors: list[str] = []

    for p in resolved_targets:
        if not p.exists():
            print(f"[WARN] Path does not exist: {p}")
            missing.append(str(p))
            continue

        try:
            if p.is_dir():
                print(f"[REMOVE] Directory: {p}")
                shutil.rmtree(p, ignore_errors=False)
            else:
                print(f"[REMOVE] File: {p}")
                p.unlink()
            deleted.append(str(p))
        except Exception as ex:
            print(f"[ERROR] Failed to delete {p}: {ex}")
            errors.append(f"{p} :: {ex}")

    status = "ok"
    if errors:
        status = "errors"
    elif not deleted:
        status = "nothing_deleted"

    print("\n=== delete_target summary ===")
    print(f"  Deleted : {len(deleted)}")
    print(f"  Missing : {len(missing)}")
    print(f"  Outside : {len(skipped_outside)}")
    print(f"  Errors  : {len(errors)}")

    print_llm_summary(
        "delete_target",
        status=status,
        PROJECT_ROOT=str(project_root),
        DELETED=deleted,
        MISSING=missing,
        SKIPPED_OUTSIDE=skipped_outside,
        ERRORS=errors,
    )

    # no interactive exit prompt for automation


if __name__ == "__main__":
    main()
