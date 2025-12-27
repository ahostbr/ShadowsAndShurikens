# sots_plugin_depmap.py
# Generate a cross-plugin dependency map for SOTS_* plugins.
# Prints a human summary and writes a JSON + log file.
#
# This script is designed to be noisy (per DevTools law): it always
# prints what it is doing and writes a small log file next to the JSON.

import json
import argparse
from pathlib import Path
from datetime import datetime


def _iter_sots_plugin_dirs(plugins_dir: Path):
    for p in sorted(plugins_dir.iterdir()):
        if p.is_dir() and p.name.startswith("SOTS"):
            yield p


def _find_uplugin_for_plugin_dir(plugin_dir: Path) -> Path | None:
    expected = plugin_dir / f"{plugin_dir.name}.uplugin"
    if expected.exists():
        return expected

    candidates = sorted(plugin_dir.glob("*.uplugin"))
    if candidates:
        return candidates[0]
    return None


def find_uplugin_files(project_root: Path):
    plugins_dir = project_root / "Plugins"
    if not plugins_dir.exists():
        raise SystemExit(f"[ERROR] Plugins dir not found under project root: {plugins_dir}")

    for plugin_dir in _iter_sots_plugin_dirs(plugins_dir):
        uplugin = _find_uplugin_for_plugin_dir(plugin_dir)
        if uplugin is not None:
            yield uplugin


def load_uplugin(path: Path):
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"[WARN] Failed to parse {path}: {e}")
        return None


def build_depmap(project_root: Path):
    plugins = {}
    for uplugin_path in find_uplugin_files(project_root):
        data = load_uplugin(uplugin_path)
        if not data:
            continue
        plugin_name = data.get("FriendlyName") or uplugin_path.stem
        plugin_key = uplugin_path.stem
        modules = data.get("Modules", [])
        plugin_deps = data.get("Plugins", [])

        dep_names = sorted({d.get("Name") for d in plugin_deps if isinstance(d, dict) and d.get("Name")})

        modules_info = []
        for m in modules:
            modules_info.append({
                "Name": m.get("Name"),
                "Type": m.get("Type"),
                "LoadingPhase": m.get("LoadingPhase"),
            })

        modules_info = sorted(modules_info, key=lambda x: (x.get("Name") or ""))

        try:
            rel_path = uplugin_path.relative_to(project_root)
            uplugin_path_str = rel_path.as_posix()
        except Exception:
            uplugin_path_str = str(uplugin_path)

        plugins[plugin_key] = {
            "FriendlyName": plugin_name,
            "Path": uplugin_path_str,
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
        help="Project root folder (must contain Plugins/)",
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
    print(f"[INFO] Scanning plugins dir: {root / 'Plugins'}")

    depmap = build_depmap(root)

    out_json.parent.mkdir(parents=True, exist_ok=True)
    with out_json.open("w", encoding="utf-8") as f:
        json.dump(depmap, f, indent=2, sort_keys=True)

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
