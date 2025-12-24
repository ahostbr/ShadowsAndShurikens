from __future__ import annotations

import argparse
from pathlib import Path

from repo_indexer import RepoIndexer, detect_project_root


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Build the SOTS repo intel index (symbols, APIs, tags, module graph)."
    )
    parser.add_argument("--project_root", help="Project root path")
    parser.add_argument("--plugins_dir", help="Plugins directory (default: <project_root>/Plugins)")
    parser.add_argument("--reports_dir", help="Reports output directory (default: <project_root>/Reports/RepoIndex)")
    parser.add_argument("--full", action="store_true", help="Ignore cache and rescan everything")
    parser.add_argument("--changed_only", action="store_true", help="Only rescan changed files (default)")
    parser.add_argument("--plugin_filter", default="", help="Optional plugin name filter (glob, comma-separated)")
    parser.add_argument("--verbose", action="store_true", help="Verbose progress output")

    args = parser.parse_args(argv)

    project_root = Path(args.project_root).resolve() if args.project_root else detect_project_root(Path(__file__).resolve())
    plugins_dir = Path(args.plugins_dir).resolve() if args.plugins_dir else project_root / "Plugins"
    reports_dir = Path(args.reports_dir).resolve() if args.reports_dir else project_root / "Reports" / "RepoIndex"

    changed_only = not args.full
    if args.changed_only:
        changed_only = True

    indexer = RepoIndexer(
        project_root=project_root,
        plugins_dir=plugins_dir,
        reports_dir=reports_dir,
        changed_only=changed_only,
        full=args.full,
        plugin_filter=args.plugin_filter,
        verbose=args.verbose,
    )
    indexer.run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
