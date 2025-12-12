#!/usr/bin/env python3
"""
devtools_selftest.py
--------------------

Quick health check for the SOTS DevTools toolbox.

It walks the DevTools/python directory (or a custom --python-dir), and for each
`.py` file:

  - In --mode compile (default): compiles the source to bytecode to catch
    syntax errors.
  - In --mode import: tries to import the module via importlib and reports any
    exceptions (including missing dependencies, bad top-level code, etc).

Results are:

  - Printed to stdout.
  - Written to DevTools/logs/devtools_selftest_YYYYMMDD_HHMMSS.log

Exit codes:
  0 -> no errors (only OK/WARN)
  1 -> at least one ERROR encountered
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import sys
import traceback
from datetime import datetime
from pathlib import Path
from typing import List, Tuple, Optional

# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

THIS_FILE = Path(__file__).resolve()
TOOLS_ROOT = THIS_FILE.parent              # .../DevTools/python
PROJECT_ROOT = TOOLS_ROOT.parent.parent    # .../ShadowsAndShurikens

DEFAULT_PYTHON_DIR = TOOLS_ROOT
DEFAULT_LOG_DIR = PROJECT_ROOT / "DevTools" / "logs"

# Default excluded directory *names* for recursive scans.
# These are simple directory-name checks, not full path patterns.
DEFAULT_EXCLUDED_DIRS: List[str] = [
    "__pycache__",
    ".git",
    ".mypy_cache",
    ".pytest_cache",
    ".conda",  # local DevTools mini-env – usually not wanted for quick selftests
]


def debug_print(msg: str) -> None:
    print(f"[devtools_selftest] {msg}")


def ensure_log_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def write_log(log_dir: Path, lines: List[str]) -> Path:
    ensure_log_dir(log_dir)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = log_dir / f"devtools_selftest_{ts}.log"
    with log_path.open("w", encoding="utf-8") as f:
        for line in lines:
            f.write(line.rstrip("\n") + "\n")
    return log_path


# ---------------------------------------------------------------------------
# Core checks
# ---------------------------------------------------------------------------

def discover_python_files(
    root: Path,
    recursive: bool,
    excluded_dirs: Optional[List[str]] = None,
) -> List[Path]:
    """
    Discover .py files under `root`.

    - If recursive is False: just root/*.py.
    - If recursive is True: walk with os.walk and skip any directory whose
      *name* is in `excluded_dirs` (if provided).
    """
    files: List[Path] = []
    excluded_names = set(excluded_dirs or [])

    if recursive:
        for dirpath, dirnames, filenames in os.walk(root):
            # In-place filter so os.walk never descends into excluded dirs.
            if excluded_names:
                dirnames[:] = [
                    d for d in dirnames
                    if d not in excluded_names
                ]

            for filename in filenames:
                if not filename.endswith(".py"):
                    continue
                p = Path(dirpath) / filename
                if p.is_file():
                    files.append(p)
    else:
        for p in root.glob("*.py"):
            if p.is_file():
                files.append(p)

    return sorted(files)


def check_compile(path: Path) -> Tuple[str, str]:
    """
    Try to compile a module.

    Returns (status, detail):
      status: "OK" or "ERROR"
      detail: message
    """
    try:
        source = path.read_text(encoding="utf-8")
    except Exception as exc:
        return "ERROR", f"Failed to read file: {exc}"

    try:
        compile(source, str(path), "exec")
    except SyntaxError as exc:
        msg = f"SyntaxError at line {exc.lineno}: {exc.msg}"
        return "ERROR", msg
    except Exception as exc:
        msg = "".join(
            traceback.format_exception_only(type(exc), exc)
        ).strip()
        return "ERROR", f"Unexpected error while compiling: {msg}"

    return "OK", "compiled successfully"


def module_name_for(path: Path, python_root: Path) -> str:
    """
    Produce a stable module-like name for importlib usage.
    """
    rel = path.relative_to(python_root)  # e.g., "subdir/foo.py"
    parts = list(rel.with_suffix("").parts)
    return "devtools_" + "_".join(parts)


def check_import(path: Path, python_root: Path) -> Tuple[str, str]:
    """
    Try to import a module from the given path.

    Returns (status, detail):
      status: "OK", "WARN", or "ERROR"
    """
    name = module_name_for(path, python_root)

    try:
        spec = importlib.util.spec_from_file_location(name, path)
        if spec is None or spec.loader is None:
            return "ERROR", "Could not create import spec/loader."

        mod = importlib.util.module_from_spec(spec)

        # Make sure the tools root is on sys.path so intra-DevTools imports work.
        if str(python_root) not in sys.path:
            sys.path.insert(0, str(python_root))

        spec.loader.exec_module(mod)  # type: ignore[attr-defined]
        return "OK", "imported successfully"

    except SystemExit as exc:
        code = exc.code
        # If a module calls sys.exit(0) on import (bad style), treat as WARN.
        return "WARN", f"SystemExit({code}) raised during import (module may auto-exit on import)."

    except Exception as exc:
        msg = "".join(
            traceback.format_exception_only(type(exc), exc)
        ).strip()
        return "ERROR", f"Exception during import: {msg}"


# ---------------------------------------------------------------------------
# Main entrypoint
# ---------------------------------------------------------------------------

def run_selftest(
    python_dir: Path,
    recursive: bool,
    mode: str,
    log_dir: Path,
    excluded_dirs: Optional[List[str]] = None,
) -> Tuple[List[str], int]:
    """
    Run the selftest and return (log_lines, num_errors).
    """
    effective_excluded = list(excluded_dirs or [])

    # Decide how to label the environment scope for the report.
    if not recursive:
        env_scope = "single-dir (non-recursive)"
    else:
        if ".conda" in effective_excluded:
            env_scope = "devtools-only ('.conda' environment skipped)"
        else:
            env_scope = "full environment (no '.conda' exclusion)"

    log_lines: List[str] = []
    log_lines.append("DEVTOOLS SELFTEST REPORT")
    log_lines.append("-" * 72)
    log_lines.append(f"Project root: {PROJECT_ROOT}")
    log_lines.append(f"Python dir: {python_dir}")
    log_lines.append(f"Mode: {mode}")
    log_lines.append(f"Recursive: {recursive}")
    log_lines.append(f"Env scope: {env_scope}")

    if recursive:
        if effective_excluded:
            log_lines.append(
                "Excluded dirs (by name): "
                + ", ".join(sorted(effective_excluded))
            )
        else:
            log_lines.append("Excluded dirs (by name): (none)")
    else:
        log_lines.append("Excluded dirs: (n/a for non-recursive scan)")

    log_lines.append("")

    if not python_dir.is_dir():
        msg = f"ERROR: python dir does not exist: {python_dir}"
        log_lines.append(msg)
        return log_lines, 1

    files = discover_python_files(
        root=python_dir,
        recursive=recursive,
        excluded_dirs=effective_excluded if recursive else None,
    )

    if not files:
        log_lines.append("No .py files found to test.")
        return log_lines, 0

    log_lines.append(f"Discovered {len(files)} .py file(s).")
    log_lines.append("")

    num_errors = 0
    num_warns = 0
    num_ok = 0

    for path in files:
        # Skip __init__ if present; it's often empty or package-only.
        if path.name == "__init__.py":
            continue

        rel = path.relative_to(PROJECT_ROOT)

        if mode == "compile":
            status, detail = check_compile(path)
        else:
            status, detail = check_import(path, python_dir)

        if status == "OK":
            num_ok += 1
        elif status == "WARN":
            num_warns += 1
        else:
            num_errors += 1

        log_lines.append(f"[{status}] {rel}")
        log_lines.append(f" -> {detail}")
        log_lines.append("")

    log_lines.append("SUMMARY")
    log_lines.append("-" * 72)
    log_lines.append(f"OK: {num_ok}")
    log_lines.append(f"WARN: {num_warns}")
    log_lines.append(f"ERROR: {num_errors}")
    log_lines.append(f"TOTAL: {num_ok + num_warns + num_errors}")
    log_lines.append(f"Env scope: {env_scope}")

    if recursive:
        if effective_excluded:
            log_lines.append(
                "Excluded dirs (by name): "
                + ", ".join(sorted(effective_excluded))
            )
        else:
            log_lines.append("Excluded dirs (by name): (none)")

    return log_lines, num_errors


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Run a health check over DevTools/python modules."
    )

    parser.add_argument(
        "--python-dir",
        type=str,
        default=str(DEFAULT_PYTHON_DIR),
        help="Root folder to scan for .py files (default: DevTools/python).",
    )
    parser.add_argument(
        "--mode",
        choices=["compile", "import"],
        default="compile",
        help="compile: just compile source; import: actually import each module.",
    )
    parser.add_argument(
        "--recursive",
        action="store_true",
        help="If set, scan subdirectories recursively.",
    )
    parser.add_argument(
        "--log-dir",
        type=str,
        default=str(DEFAULT_LOG_DIR),
        help="Directory for selftest logs (default: /DevTools/logs).",
    )
    parser.add_argument(
        "--exclude-dir",
        action="append",
        default=None,
        help=(
            "Directory name to exclude during recursive walk "
            "(e.g. .conda, __pycache__). "
            "May be passed multiple times."
        ),
    )
    parser.add_argument(
        "--no-default-excludes",
        action="store_true",
        help=(
            "If set, do not apply DEFAULT_EXCLUDED_DIRS. "
            "Use this if you want a full-environment run."
        ),
    )

    args = parser.parse_args(argv)

    python_dir = Path(args.python_dir).resolve()
    log_dir = Path(args.log_dir).resolve()

    # Build the effective exclude list.
    cli_excludes = list(args.exclude_dir or [])
    if args.no_default_excludes:
        excluded_dirs: List[str] = cli_excludes
    else:
        excluded_dirs = list(DEFAULT_EXCLUDED_DIRS)
        for name in cli_excludes:
            if name not in excluded_dirs:
                excluded_dirs.append(name)

    debug_print(f"Python dir: {python_dir}")
    debug_print(f"Mode: {args.mode}")
    debug_print(f"Recursive: {args.recursive}")
    debug_print(f"Log dir: {log_dir}")
    if args.recursive:
        debug_print(f"Excluded dirs (by name): {excluded_dirs if excluded_dirs else '(none)'}")
    else:
        debug_print("Excluded dirs: (n/a for non-recursive scan)")

    lines, num_errors = run_selftest(
        python_dir=python_dir,
        recursive=args.recursive,
        mode=args.mode,
        log_dir=log_dir,
        excluded_dirs=excluded_dirs if args.recursive else None,
    )

    # Print to console
    for line in lines:
        print(line)

    # Write to log
    log_path = write_log(log_dir, lines)
    debug_print(f"Selftest log written to: {log_path}")

    return 0 if num_errors == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
