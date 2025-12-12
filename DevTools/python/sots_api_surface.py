# sots_api_surface.py
# Extract a snapshot of the public API surface (UCLASS/USTRUCT/UENUM and
# Blueprint-exposed UFUNCTION/UPROPERTY) for a given plugin or source root.
#
# Noisy by design: prints what it is doing and writes a JSON + log.

import os
import re
import json
import argparse
from pathlib import Path
from datetime import datetime

RE_TYPE = re.compile(r"U(CLASS|STRUCT|ENUM)\s*\(.*?\)\s*(class|struct|enum)\s+(\w+)", re.DOTALL)
RE_UFUNCTION = re.compile(r"UFUNCTION\s*\((.*?)\)\s*(.*?)\s+(\w+)\s*\(", re.DOTALL)
RE_UPROPERTY = re.compile(r"UPROPERTY\s*\((.*?)\)\s*(.*?);", re.DOTALL)


def scan_header(path: Path):
    result = {
        "types": [],
        "functions": [],
        "properties": [],
    }
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        print(f"[WARN] Failed to read {path}: {e}")
        return result

    for m in RE_TYPE.finditer(text):
        utype, kw, name = m.groups()
        result["types"].append({
            "kind": f"U{utype}",
            "name": name,
        })

    for m in RE_UFUNCTION.finditer(text):
        meta, rest, name = m.groups()
        if "Blueprint" in meta:
            result["functions"].append({
                "name": name,
                "meta": meta.strip(),
                "declaration": rest.strip(),
            })

    for m in RE_UPROPERTY.finditer(text):
        meta, decl = m.groups()
        if "Blueprint" in meta:
            result["properties"].append({
                "meta": meta.strip(),
                "declaration": decl.strip(),
            })

    return result


def build_api_surface(root: Path, plugin_name: str, exts):
    data = {}
    total_files = 0
    for dirpath, dirnames, filenames in os.walk(root):
        for fname in filenames:
            p = Path(dirpath) / fname
            if p.suffix not in exts:
                continue
            total_files += 1
            rel = p.relative_to(root)
            scan_res = scan_header(p)
            if not (scan_res["types"] or scan_res["functions"] or scan_res["properties"]):
                continue
            data[str(rel)] = scan_res
    return data, total_files


def main():
    parser = argparse.ArgumentParser(description="SOTS API surface snapshot")
    parser.add_argument(
        "--root",
        type=str,
        required=True,
        help="Root source folder (e.g. Plugins/SOTS_Parkour/Source/SOTS_Parkour)",
    )
    parser.add_argument(
        "--plugin-name",
        type=str,
        default=None,
        help="Optional plugin name label for reporting.",
    )
    parser.add_argument(
        "--output-json",
        type=str,
        default="DevTools/reports/sots_api_surface.json",
        help="Where to write JSON output.",
    )
    args = parser.parse_args()

    root = Path(args.root).resolve()
    plugin_name = args.plugin_name or root.name
    out_json = Path(args.output_json).resolve()
    out_log = out_json.with_suffix(".log")

    print(f"[INFO] sots_api_surface.py starting at {datetime.now().isoformat()}")
    print(f"[INFO] Root: {root}")
    print(f"[INFO] Plugin: {plugin_name}")

    exts = {".h", ".hpp"}
    api_data, total_files = build_api_surface(root, plugin_name, exts)

    out_json.parent.mkdir(parents=True, exist_ok=True)
    with out_json.open("w", encoding="utf-8") as f:
        json.dump({
            "plugin": plugin_name,
            "root": str(root),
            "total_files": total_files,
            "files": api_data,
        }, f, indent=2)

    with out_log.open("w", encoding="utf-8") as f:
        f.write(f"sots_api_surface run at {datetime.now().isoformat()}\n")
        f.write(f"Root: {root}\n")
        f.write(f"Plugin: {plugin_name}\n")
        f.write(f"Total header files scanned: {total_files}\n")
        f.write(f"Files with API data: {len(api_data)}\n")

    print(f"[RESULT] Headers scanned: {total_files}")
    print(f"[RESULT] Files with API data: {len(api_data)}")
    print(f"[INFO] JSON written to: {out_json}")
    print(f"[INFO] Log written to: {out_log}")
    print("[INFO] Done.")


if __name__ == "__main__":
    main()
