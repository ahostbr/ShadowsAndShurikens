#!/usr/bin/env python3
"""
sots_plugin_discovery.py

Universal Unreal plugin discovery tool for SOTS DevTools.

- Accepts either:
    --plugin-root <path/to/PluginFolder>
    OR
    --plugin-zip <path/to/PluginArchive.zip>

- Locates the .uplugin file, parses metadata.
- Scans Source/* modules (Public/Private) for .h/.hpp/.cpp.
- Uses regex to find UCLASS/USTRUCT/UENUM declarations.
- Emits a Markdown discovery doc and a small .log file.
- Prints a short summary to stdout (never runs silently).

Intended usage example:

    python sots_plugin_discovery.py --plugin-root Plugins/SOTS_Parkour

    python sots_plugin_discovery.py --plugin-zip SOTS_ParkourV_0.2.zip \
        --output-doc Plugins/SOTS_Parkour/Docs/SOTS_Parkour_Discovery.md
"""

import argparse
import datetime
import json
import os
import re
import sys
import textwrap
import zipfile
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any


# -----------------------
# Helpers: IO & utilities
# -----------------------

def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def safe_json_loads(s: str) -> Optional[Dict[str, Any]]:
    try:
        return json.loads(s)
    except Exception:
        return None


def now_iso() -> str:
    return datetime.datetime.now().isoformat(timespec="seconds")


# -----------------------
# UPlugin metadata
# -----------------------

def find_uplugin_in_root(plugin_root: Path) -> Optional[Path]:
    candidates = list(plugin_root.glob("*.uplugin"))
    return candidates[0] if candidates else None


def find_uplugin_in_zip(z: zipfile.ZipFile) -> Optional[str]:
    for name in z.namelist():
        if name.lower().endswith(".uplugin"):
            return name
    return None


def parse_uplugin_text(text: str) -> Dict[str, Any]:
    data = safe_json_loads(text)
    if not isinstance(data, dict):
        return {}
    return data


def summarize_uplugin(uplugin_data: Dict[str, Any]) -> Dict[str, Any]:
    # Extract some common fields; tolerate missing keys.
    return {
        "FileVersion": uplugin_data.get("FileVersion"),
        "Version": uplugin_data.get("Version"),
        "VersionName": uplugin_data.get("VersionName"),
        "FriendlyName": uplugin_data.get("FriendlyName"),
        "Description": uplugin_data.get("Description"),
        "Category": uplugin_data.get("Category"),
        "CreatedBy": uplugin_data.get("CreatedBy"),
        "CreatedByURL": uplugin_data.get("CreatedByURL"),
        "DocsURL": uplugin_data.get("DocsURL"),
        "MarketplaceURL": uplugin_data.get("MarketplaceURL"),
        "Modules": uplugin_data.get("Modules", []),
        "Plugins": uplugin_data.get("Plugins", []),
    }


# -----------------------
# Source scanning (folder)
# -----------------------

HEADER_EXTS = {".h", ".hpp"}
SOURCE_EXTS = {".cpp", ".cc", ".cxx"}


def scan_source_tree(plugin_root: Path) -> Dict[str, Any]:
    """
    Scan Source/* modules for .h/.cpp, grouped by module and visibility.
    """
    source_root = plugin_root / "Source"
    modules: Dict[str, Dict[str, List[Path]]] = {}
    if not source_root.is_dir():
        return {"modules": modules, "header_count": 0, "source_count": 0}

    header_count = 0
    source_count = 0

    for module_dir in source_root.iterdir():
        if not module_dir.is_dir():
            continue
        module_name = module_dir.name
        module_info = {"Public": [], "Private": [], "Other": []}

        for root, dirs, files in os.walk(module_dir):
            root_path = Path(root)
            # Determine "visibility" based on folder name under module.
            if root_path.name == "Public":
                vis = "Public"
            elif root_path.name == "Private":
                vis = "Private"
            else:
                vis = "Other"

            for fname in files:
                ext = Path(fname).suffix.lower()
                if ext not in HEADER_EXTS and ext not in SOURCE_EXTS:
                    continue
                fpath = root_path / fname
                module_info[vis].append(fpath.relative_to(plugin_root))

                if ext in HEADER_EXTS:
                    header_count += 1
                else:
                    source_count += 1

        modules[module_name] = module_info

    return {
        "modules": modules,
        "header_count": header_count,
        "source_count": source_count,
    }


# -----------------------
# Source scanning (zip)
# -----------------------

def scan_source_zip(z: zipfile.ZipFile, uplugin_name: str) -> Dict[str, Any]:
    """
    Scan Source/* modules inside a zip, grouped by module and visibility.
    We assume the .uplugin lives at <PluginName>.uplugin or some folder
    such that Source/ is at the same level as the .uplugin.
    """
    # Infer plugin root inside zip (prefix)
    # E.g. MyPlugin/MyPlugin.uplugin -> plugin_prefix="MyPlugin/"
    plugin_prefix = ""
    if "/" in uplugin_name:
        plugin_prefix = uplugin_name.rsplit("/", 1)[0] + "/"

    modules: Dict[str, Dict[str, List[str]]] = {}
    header_count = 0
    source_count = 0

    for name in z.namelist():
        if not name.startswith(plugin_prefix):
            continue
        rel = name[len(plugin_prefix):]  # relative to plugin root
        # Expect "Source/ModuleName/..." path for source files
        parts = rel.split("/")
        if len(parts) < 3:
            continue
        if parts[0] != "Source":
            continue

        module_name = parts[1]
        vis_dir = parts[2] if len(parts) >= 3 else ""
        vis = "Other"
        if vis_dir == "Public":
            vis = "Public"
        elif vis_dir == "Private":
            vis = "Private"

        ext = Path(name).suffix.lower()
        if ext not in HEADER_EXTS and ext not in SOURCE_EXTS:
            continue

        modules.setdefault(module_name, {"Public": [], "Private": [], "Other": []})
        modules[module_name][vis].append(rel)

        if ext in HEADER_EXTS:
            header_count += 1
        else:
            source_count += 1

    return {
        "modules": modules,
        "header_count": header_count,
        "source_count": source_count,
        "plugin_prefix": plugin_prefix,
    }


# -----------------------
# Macro scanning (folder)
# -----------------------

UCLASS_PATTERN = re.compile(
    r"^\s*UCLASS\s*\([^)]*\)\s*class\s+[\w_]+\s+(\w+)\s*:",
    re.MULTILINE
)
USTRUCT_PATTERN = re.compile(
    r"^\s*USTRUCT\s*\([^)]*\)\s*struct\s+[\w_]+\s+(\w+)\s*",
    re.MULTILINE
)
UENUM_PATTERN = re.compile(
    r"^\s*UENUM[^{]*\{?\s*.*?\n\s*(enum\s+class\s+(\w+)\b|enum\s+(\w+)\b)",
    re.MULTILINE
)


def scan_macros_in_file_text(text: str) -> Dict[str, List[str]]:
    uclasses = UCLASS_PATTERN.findall(text)
    ustructs = USTRUCT_PATTERN.findall(text)
    uenums_raw = UENUM_PATTERN.findall(text)
    uenums: List[str] = []
    for full_match, enum_class_name, enum_name in uenums_raw:
        if enum_class_name:
            uenums.append(enum_class_name)
        elif enum_name:
            uenums.append(enum_name)

    return {
        "uclasses": uclasses,
        "ustructs": ustructs,
        "uenums": uenums,
    }


def scan_macros_in_root(plugin_root: Path) -> Dict[str, List[str]]:
    uclasses: List[str] = []
    ustructs: List[str] = []
    uenums: List[str] = []

    for root, dirs, files in os.walk(plugin_root / "Source"):
        for fname in files:
            ext = Path(fname).suffix.lower()
            if ext not in HEADER_EXTS and ext not in SOURCE_EXTS:
                continue
            path = Path(root) / fname
            text = read_text(path)
            result = scan_macros_in_file_text(text)
            uclasses.extend(result["uclasses"])
            ustructs.extend(result["ustructs"])
            uenums.extend(result["uenums"])

    return {
        "uclasses": sorted(set(uclasses)),
        "ustructs": sorted(set(ustructs)),
        "uenums": sorted(set(uenums)),
    }


# -----------------------
# Macro scanning (zip)
# -----------------------

def scan_macros_in_zip(z: zipfile.ZipFile, plugin_prefix: str) -> Dict[str, List[str]]:
    uclasses: List[str] = []
    ustructs: List[str] = []
    uenums: List[str] = []

    for name in z.namelist():
        if not name.startswith(plugin_prefix + "Source/"):
            continue
        ext = Path(name).suffix.lower()
        if ext not in HEADER_EXTS and ext not in SOURCE_EXTS:
            continue
        try:
            data = z.read(name)
        except KeyError:
            continue
        text = data.decode("utf-8", errors="ignore")
        result = scan_macros_in_file_text(text)
        uclasses.extend(result["uclasses"])
        ustructs.extend(result["ustructs"])
        uenums.extend(result["uenums"])

    return {
        "uclasses": sorted(set(uclasses)),
        "ustructs": sorted(set(ustructs)),
        "uenums": sorted(set(uenums)),
    }


# -----------------------
# Markdown / log generation
# -----------------------

def guess_plugin_name_from_uplugin_path(uplugin_path: Path) -> str:
    return uplugin_path.stem


def guess_plugin_name_from_zip_entry(uplugin_name: str) -> str:
    # e.g. "MyPlugin/MyPlugin.uplugin" -> "MyPlugin"
    base = uplugin_name.rsplit("/", 1)[-1]
    return Path(base).stem


def render_markdown(
    plugin_name: str,
    uplugin_summary: Dict[str, Any],
    source_summary: Dict[str, Any],
    macro_summary: Dict[str, List[str]],
    from_zip: bool,
) -> str:
    meta_lines = []
    for k in [
        "FileVersion", "Version", "VersionName", "FriendlyName",
        "Description", "Category", "CreatedBy", "CreatedByURL",
        "DocsURL", "MarketplaceURL",
    ]:
        v = uplugin_summary.get(k)
        if v is not None:
            meta_lines.append(f"- **{k}**: {v}")

    modules = source_summary.get("modules", {})
    header_count = source_summary.get("header_count", 0)
    source_count = source_summary.get("source_count", 0)

    lines = []
    lines.append(f"# Plugin Discovery – {plugin_name}")
    lines.append("")
    lines.append(f"- Generated: `{now_iso()}`")
    lines.append(f"- Source: {'ZIP archive' if from_zip else 'Plugin folder'}")
    lines.append("")
    lines.append("## UPlugin Metadata")
    lines.append("")
    if meta_lines:
        lines.extend(meta_lines)
    else:
        lines.append("_No metadata fields could be parsed._")
    lines.append("")

    lines.append("## Modules & Source Layout")
    lines.append("")
    lines.append(f"- Total header files: **{header_count}**")
    lines.append(f"- Total source files: **{source_count}**")
    lines.append("")
    if not modules:
        lines.append("_No modules found under Source/._")
    else:
        for module_name, info in sorted(modules.items()):
            lines.append(f"### Module: `{module_name}`")
            lines.append("")
            for vis in ["Public", "Private", "Other"]:
                entries = info.get(vis, [])
                if not entries:
                    continue
                lines.append(f"- **{vis}** ({len(entries)} files):")
                for rel in sorted(entries):
                    lines.append(f"  - `{rel}`")
            lines.append("")

    lines.append("## Reflected Types")
    lines.append("")
    lines.append(f"- UCLASS types: {len(macro_summary.get('uclasses', []))}")
    for n in macro_summary.get("uclasses", []):
        lines.append(f"  - `{n}`")
    lines.append("")
    lines.append(f"- USTRUCT types: {len(macro_summary.get('ustructs', []))}")
    for n in macro_summary.get("ustructs", []):
        lines.append(f"  - `{n}`")
    lines.append("")
    lines.append(f"- UENUM types: {len(macro_summary.get('uenums', []))}")
    for n in macro_summary.get("uenums", []):
        lines.append(f"  - `{n}`")
    lines.append("")

    lines.append("## Notes")
    lines.append("")
    lines.append(
        textwrap.dedent(
            """
            - This document is generated by `sots_plugin_discovery.py`.
            - It is intended as a high-level inventory for DevTools and SOTS_DEVTOOLS passes.
            - You can safely regenerate this file after code changes.
            """
        ).strip()
    )
    lines.append("")
    return "\n".join(lines)


def write_text_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def write_log(log_path: Path, message: str) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("a", encoding="utf-8") as f:
        f.write(f"[{now_iso()}] {message}\n")


# -----------------------
# Main orchestration
# -----------------------

def run_for_plugin_root(plugin_root: Path, output_doc: Optional[Path]) -> Tuple[Optional[Path], Optional[Path]]:
    uplugin_path = find_uplugin_in_root(plugin_root)
    if not uplugin_path:
        print(f"[ERROR] No .uplugin file found in plugin root: {plugin_root}", file=sys.stderr)
        return None, None

    plugin_name = guess_plugin_name_from_uplugin_path(uplugin_path)
    uplugin_data = parse_uplugin_text(read_text(uplugin_path))
    summary = summarize_uplugin(uplugin_data)
    source_summary = scan_source_tree(plugin_root)
    macro_summary = scan_macros_in_root(plugin_root)

    if output_doc is None:
        docs_root = plugin_root / "Docs"
        output_doc = docs_root / f"{plugin_name}_Discovery.md"

    markdown = render_markdown(plugin_name, summary, source_summary, macro_summary, from_zip=False)
    write_text_file(output_doc, markdown)

    log_path = output_doc.with_suffix(".log")
    write_log(log_path, f"Generated discovery doc for plugin root: {plugin_root}")

    return output_doc, log_path


def run_for_plugin_zip(zip_path: Path, output_doc: Optional[Path]) -> Tuple[Optional[Path], Optional[Path]]:
    if not zip_path.is_file():
        print(f"[ERROR] Zip file not found: {zip_path}", file=sys.stderr)
        return None, None

    with zipfile.ZipFile(zip_path, "r") as z:
        uplugin_name = find_uplugin_in_zip(z)
        if not uplugin_name:
            print(f"[ERROR] No .uplugin file found in zip: {zip_path}", file=sys.stderr)
            return None, None

        data = z.read(uplugin_name).decode("utf-8", errors="ignore")
        plugin_name = guess_plugin_name_from_zip_entry(uplugin_name)
        uplugin_data = parse_uplugin_text(data)
        summary = summarize_uplugin(uplugin_data)

        source_summary = scan_source_zip(z, uplugin_name)
        plugin_prefix = source_summary.get("plugin_prefix", "")
        macro_summary = scan_macros_in_zip(z, plugin_prefix)

    if output_doc is None:
        # For zips, default to CWD.
        output_doc = Path.cwd() / f"{plugin_name}_Discovery_from_zip.md"

    markdown = render_markdown(plugin_name, summary, source_summary, macro_summary, from_zip=True)
    write_text_file(output_doc, markdown)

    log_path = output_doc.with_suffix(".log")
    write_log(log_path, f"Generated discovery doc from zip: {zip_path}")

    return output_doc, log_path


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Universal Unreal plugin discovery tool for SOTS DevTools."
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--plugin-root",
        type=str,
        help="Path to plugin folder (contains .uplugin, Source/ etc.)",
    )
    group.add_argument(
        "--plugin-zip",
        type=str,
        help="Path to plugin zip archive (contains .uplugin and Source/).",
    )
    parser.add_argument(
        "--output-doc",
        type=str,
        help="Optional explicit output path for the Markdown discovery doc.",
    )
    return parser.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)

    output_doc: Optional[Path] = Path(args.output_doc) if args.output_doc else None

    if args.plugin_root:
        plugin_root = Path(args.plugin_root).resolve()
        print(f"[INFO] Running plugin discovery for plugin root: {plugin_root}")
        doc_path, log_path = run_for_plugin_root(plugin_root, output_doc)
    else:
        zip_path = Path(args.plugin_zip).resolve()
        print(f"[INFO] Running plugin discovery for plugin zip: {zip_path}")
        doc_path, log_path = run_for_plugin_zip(zip_path, output_doc)

    if doc_path is None or log_path is None:
        print("[ERROR] Plugin discovery failed. See messages above.", file=sys.stderr)
        return 1

    print("")
    print("[RESULT] Discovery complete.")
    print(f"- Doc written to: {doc_path}")
    print(f"- Log written to: {log_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
