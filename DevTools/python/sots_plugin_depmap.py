# sots_plugin_depmap.py
# Generate a cross-plugin dependency map for SOTS_* plugins.
# Prints a human summary and writes a JSON + log file.
#
# This script is designed to be noisy (per DevTools law): it always
# prints what it is doing and writes a small log file next to the JSON.

import os
import json
import argparse
from pathlib import Path
from datetime import datetime


def find_uplugin_files(root: Path):
    for p in root.rglob("*.uplugin"):
        # Only consider SOTS_* plugins by default
        if "SOTS_" in p.name or "SOTS" in p.parent.name:
            yield p


def load_uplugin(path: Path):
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"[WARN] Failed to parse {path}: {e}")
        return None


def build_depmap(root: Path):
    plugins = {}
    for uplugin_path in find_uplugin_files(root):
        data = load_uplugin(uplugin_path)
        if not data:
            continue
        plugin_name = data.get("FriendlyName") or uplugin_path.stem
        plugin_key = uplugin_path.stem
        modules = data.get("Modules", [])
        plugin_deps = data.get("Plugins", [])

        dep_names = [d.get("Name") for d in plugin_deps if "Name" in d]

        modules_info = []
        for m in modules:
            modules_info.append({
                "Name": m.get("Name"),
                "Type": m.get("Type"),
                "LoadingPhase": m.get("LoadingPhase"),
            })

        plugins[plugin_key] = {
            "FriendlyName": plugin_name,
            "Path": str(uplugin_path),
            "Dependencies": dep_names,
            "Modules": modules_info,
        }

    return plugins


def summarize_depmap(depmap: dict):
    print("[RESULT] Plugin Dependency Map")
    print("================================")
    for key, info in sorted(depmap.items()):
        print(f"- {key} ({info.get('FriendlyName','')})")
        deps = info.get("Dependencies") or []
        if deps:
            print(f"    Depends on: {', '.join(deps)}")
        else:
            print("    Depends on: (none listed)")


def main():
    parser = argparse.ArgumentParser(description="SOTS plugin dependency map generator")
    parser.add_argument(
        "--root",
        type=str,
        default=".",
        help="Root folder to scan (e.g. project root containing Plugins/)",
    )
    parser.add_argument(
        "--output-json",
        type=str,
        default="DevTools/reports/sots_plugin_depmap.json",
        help="Path to write JSON output (will create directories).",
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    out_json = Path(args.output_json).resolve()
    out_log = out_json.with_suffix(".log")

    print(f"[INFO] sots_plugin_depmap.py starting at {datetime.now().isoformat()}")
    print(f"[INFO] Scanning root: {root}")

    depmap = build_depmap(root)

    out_json.parent.mkdir(parents=True, exist_ok=True)
    with out_json.open("w", encoding="utf-8") as f:
        json.dump(depmap, f, indent=2)

    summarize_depmap(depmap)

    with out_log.open("w", encoding="utf-8") as f:
        f.write(f"sots_plugin_depmap run at {datetime.now().isoformat()}\n")
        f.write(f"Root: {root}\n")
        f.write(f"Plugins found: {len(depmap)}\n")

    print(f"[INFO] JSON written to: {out_json}")
    print(f"[INFO] Log written to: {out_log}")
    print("[INFO] Done.")


if __name__ == "__main__":
    main()
