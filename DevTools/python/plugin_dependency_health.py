#!/usr/bin/env python3
"""
plugin_dependency_health.py

Run a set of plugin dependency health checks for the SOTS project.
"""
from __future__ import annotations

import argparse
from pathlib import Path

from project_paths import get_project_root, get_plugins_dir
from plugin_audit import audit_plugins
from ensure_plugin_modules import load_config, ensure_module_for_plugin
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Run plugin dependency health checks (inventory + module verification)."
    )
    parser.add_argument(
        "--config",
        type=str,
        default="sots_plugin_modules.json",
        help="Path to plugin/module config JSON (relative to DevTools/python unless absolute).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Only report what would be changed; do not modify any files.",
    )
    args = parser.parse_args()

    confirm_start("plugin_dependency_health")

    tools_root = Path(__file__).resolve().parent
    project_root = get_project_root()
    plugins_dir = get_plugins_dir()

    cfg_path = Path(args.config)
    if not cfg_path.is_absolute():
        cfg_path = tools_root / cfg_path

    print(f"[INFO] Tools root   : {tools_root}")
    print(f"[INFO] Project root : {project_root}")
    print(f"[INFO] Plugins dir  : {plugins_dir}")
    print(f"[INFO] Config JSON  : {cfg_path}")

    status = "ok"
    total = ok = fixed = 0

    try:
        print("\n=== Plugin inventory ===\n")
        audit_plugins(plugins_dir, show_modules=False)
    except Exception as ex:
        print(f"[ERROR] plugin_audit failed: {ex}")
        status = "audit_failed"

    config = load_config(cfg_path)
    plugins_cfg = config.get("plugins", [])
    default_template = ""
    dt = config.get("default_template")
    if isinstance(dt, dict):
        default_template = dt.get("body", "")

    if not plugins_cfg:
        print("\n[WARN] No 'plugins' list found in config; skipping module verification.")
        if status == "ok":
            status = "no_plugins_list"
    elif not default_template:
        print("\n[WARN] No 'default_template.body' found in config; skipping module verification.")
        if status == "ok":
            status = "no_template"
    else:
        print("\n=== Verifying module .cpp files for listed plugins ===\n")
        for entry in plugins_cfg:
            plugin_name = entry.get("plugin_name")
            if not plugin_name:
                continue

            plugin_dir = plugins_dir / plugin_name
            if not plugin_dir.exists():
                print(f"[WARN] Plugin listed in config but folder missing: {plugin_dir}")
                continue

            total += 1
            exists, did_fix = ensure_module_for_plugin(
                plugin_root=plugins_dir,
                plugin_desc=entry,
                default_template=default_template,
                dry_run=args.dry_run,
            )

            if exists:
                ok += 1
            if did_fix:
                fixed += 1

    print("\n=== Summary ===")
    print(f"  Plugins in config : {total}")
    print(f"  Modules present   : {ok}")
    print(f"  Modules patched   : {fixed}")
    print(f"  Dry-run           : {args.dry_run}")

    if status == "ok" and total == 0:
        status = "no_plugins_in_config"

    print_llm_summary(
        "plugin_dependency_health",
        status=status,
        TOOLS_ROOT=str(tools_root),
        PROJECT_ROOT=str(project_root),
        PLUGINS_DIR=str(plugins_dir),
        CONFIG=str(cfg_path),
        PLUGINS_TOTAL=total,
        PLUGINS_OK=ok,
        PLUGINS_FIXED=fixed,
        DRY_RUN=args.dry_run,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
