from __future__ import annotations

import argparse
import json
import os
import re
import sys
import zipfile
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable, List, Optional, Set, Tuple


SCRIPT_VERSION = "1.0.1"

# Fallback allowlist (used if laws parse fails). This is SAFE: we still only include
# plugins that actually exist under Plugins/ at runtime.
FALLBACK_CANONICAL_SUITE: Set[str] = {
    "BEP",
    "BlueprintCommentLinks",
    "LightProbePlugin",
    "OmniTrace",
    "SOTS_AIPerception",
    "SOTS_BlueprintGen",
    "SOTS_BodyDrag",
    "SOTS_Debug",
    "SOTS_EdUtil",
    "SOTS_FX_Plugin",
    "SOTS_GAS_Plugin",
    "SOTS_GlobalStealthManager",
    "SOTS_Interaction",
    "SOTS_INV",
    "SOTS_KillExecutionManager",
    "SOTS_MissionDirector",
    "SOTS_MMSS",
    "SOTS_Parkour",
    "SOTS_ProfileShared",
    "SOTS_SkillTree",
    "SOTS_Steam",
    "SOTS_Stats",
    "SOTS_TagManager",
    "SOTS_UDSBridge",
    "SOTS_UI",
}


@dataclass
class PluginInfo:
    folder: Path
    uplugin_files: List[Path]
    uplugin_stems: List[str]


def now_stamp() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def print_and_log(log_lines: List[str], msg: str) -> None:
    print(msg)
    log_lines.append(msg)


def find_project_root(start: Path) -> Path:
    """
    Attempts to find a folder that contains 'Plugins/' by walking upward a bit.
    This makes it safe to run from:
      - project root
      - DevTools/python
      - anywhere under the repo
    """
    start = start.resolve()
    candidates = [start] + list(start.parents)[:6]
    for c in candidates:
        if (c / "Plugins").is_dir():
            return c
    return start


def find_laws_file(project_root: Path) -> Optional[Path]:
    # Common locations used in this repo
    candidates = [
        project_root / "SOTS_Suite_ExtendedMemory_LAWs.txt",
        project_root / "Docs" / "SOTS_Suite_ExtendedMemory_LAWs.txt",
        project_root / "Plugins" / "SOTS_TagManager" / "Docs" / "SOTS_Suite_ExtendedMemory_LAWs.txt",
        project_root / "Plugins" / "SOTS_TagManager" / "Docs" / "Laws" / "SOTS_Suite_ExtendedMemory_LAWs.txt",
    ]
    for p in candidates:
        if p.is_file():
            return p
    return None


def parse_canonical_suite_from_laws(laws_path: Path) -> Optional[Set[str]]:
    """
    Parses a section like:
      "The suite contains these 23 plugins (by .uplugin)"
      - PluginA
      - PluginB
    Returns the set, or None if not found/parseable.
    """
    try:
        text = laws_path.read_text(encoding="utf-8", errors="replace").splitlines()
    except Exception:
        return None

    anchor_re = re.compile(r"suite contains these\s+\d+\s+plugins\s+\(by\s+\.uplugin\)", re.IGNORECASE)
    anchor_idx = None
    for i, line in enumerate(text):
        if anchor_re.search(line):
            anchor_idx = i
            break
    if anchor_idx is None:
        return None

    names: Set[str] = set()
    item_re = re.compile(r"^\s*-\s*([A-Za-z0-9_]+)\s*$")
    for line in text[anchor_idx + 1 : anchor_idx + 300]:
        m = item_re.match(line)
        if m:
            names.add(m.group(1))
            continue
        # stop once we've started collecting and the list ends
        if names and line.strip() and not line.lstrip().startswith("-"):
            break

    return names or None


def discover_plugins(plugins_dir: Path) -> List[PluginInfo]:
    plugins: List[PluginInfo] = []
    for child in sorted(plugins_dir.iterdir()):
        if not child.is_dir():
            continue
        uplugins = sorted(child.glob("*.uplugin"))
        if not uplugins:
            continue
        stems = [p.stem for p in uplugins]
        plugins.append(PluginInfo(folder=child, uplugin_files=uplugins, uplugin_stems=stems))
    return plugins


def should_include_plugin(plugin: PluginInfo, canonical_suite: Set[str], extra_allow: Set[str]) -> bool:
    folder_name = plugin.folder.name
    stems = plugin.uplugin_stems

    # Primary include rule: SOTS-prefixed (folder or uplugin stem)
    if folder_name.startswith("SOTS_"):
        return True
    if any(s.startswith("SOTS_") for s in stems):
        return True

    # Secondary include rule: in canonical suite allowlist (covers BEP/OmniTrace/etc.)
    if folder_name in canonical_suite:
        return True
    if any(s in canonical_suite for s in stems):
        return True

    # Extra allowlist (manual)
    if folder_name in extra_allow:
        return True
    if any(s in extra_allow for s in stems):
        return True

    return False


def iter_files_recursive(root: Path) -> Iterable[Path]:
    for p in root.rglob("*"):
        if p.is_file():
            yield p


def safe_arcname(project_root: Path, file_path: Path) -> str:
    """
    Zip internal path: prefer project-root-relative to keep structure stable.
    """
    rel = file_path.resolve().relative_to(project_root.resolve())
    return str(rel).replace("\\", "/")


def build_zip_bundle(
    project_root: Path,
    out_zip: Path,
    canonical_suite: Set[str],
    extra_allow: Set[str],
    dry_run: bool,
    log_lines: List[str],
) -> int:
    plugins_dir = project_root / "Plugins"
    plugins = discover_plugins(plugins_dir)

    included: List[PluginInfo] = []
    excluded: List[PluginInfo] = []
    for p in plugins:
        if should_include_plugin(p, canonical_suite, extra_allow):
            included.append(p)
        else:
            excluded.append(p)

    print_and_log(log_lines, f"[INFO] zip_sots_suite_plugin_sources.py v{SCRIPT_VERSION}")
    print_and_log(log_lines, f"[INFO] ProjectRoot: {project_root}")
    print_and_log(log_lines, f"[INFO] PluginsDir : {plugins_dir}")
    print_and_log(log_lines, f"[INFO] Discovered plugins (with .uplugin): {len(plugins)}")
    print_and_log(log_lines, f"[INFO] Included (SOTS_* + suite allowlist): {len(included)}")
    print_and_log(log_lines, f"[INFO] Excluded: {len(excluded)}")

    if not included:
        print_and_log(log_lines, "[ERROR] No plugins matched include rules. Nothing to do.")
        return 2

    files_to_add: List[Tuple[Path, str]] = []
    warnings: List[str] = []

    for p in included:
        # add all *.uplugin in the plugin root
        for u in p.uplugin_files:
            files_to_add.append((u, safe_arcname(project_root, u)))

        # add Source/** only (zip ONLY .uplugin + Source)
        src = p.folder / "Source"
        if src.is_dir():
            for f in iter_files_recursive(src):
                files_to_add.append((f, safe_arcname(project_root, f)))
        else:
            warnings.append(f"[WARN] Missing Source/ for plugin folder: {p.folder}")

    # Dedup (by resolved path)
    seen: Set[Path] = set()
    deduped: List[Tuple[Path, str]] = []
    for fp, arc in files_to_add:
        r = fp.resolve()
        if r in seen:
            continue
        seen.add(r)
        deduped.append((fp, arc))

    total_bytes = 0
    for fp, _ in deduped:
        try:
            total_bytes += fp.stat().st_size
        except Exception:
            pass

    print_and_log(log_lines, f"[INFO] Total files to add: {len(deduped)}")
    print_and_log(log_lines, f"[INFO] Approx input bytes : {total_bytes:,}")

    if warnings:
        for w in warnings:
            print_and_log(log_lines, w)

    manifest = {
        "script": "zip_sots_suite_plugin_sources.py",
        "script_version": SCRIPT_VERSION,
        "timestamp": datetime.now().isoformat(),
        "project_root": str(project_root),
        "plugins_dir": str(plugins_dir),
        "dry_run": bool(dry_run),
        "canonical_suite_count": len(canonical_suite),
        "canonical_suite_source": "laws_parsed" if canonical_suite != FALLBACK_CANONICAL_SUITE else "fallback_hardcoded",
        "extra_allow": sorted(extra_allow),
        "included_plugins": [
            {
                "folder": str(p.folder.relative_to(project_root)),
                "uplugin_files": [str(u.relative_to(project_root)) for u in p.uplugin_files],
                "uplugin_stems": p.uplugin_stems,
            }
            for p in included
        ],
        "excluded_plugins": [
            {
                "folder": str(p.folder.relative_to(project_root)),
                "uplugin_stems": p.uplugin_stems,
            }
            for p in excluded
        ],
        "warnings": warnings,
        "file_count": len(deduped),
        "approx_input_bytes": total_bytes,
    }

    if dry_run:
        print_and_log(log_lines, "[DRYRUN] Not writing zip. Showing first 50 entries:")
        for _, arc in deduped[:50]:
            print_and_log(log_lines, f"  - {arc}")
        if len(deduped) > 50:
            print_and_log(log_lines, f"  ... ({len(deduped) - 50} more)")
        return 0

    out_zip.parent.mkdir(parents=True, exist_ok=True)
    print_and_log(log_lines, f"[INFO] Writing zip: {out_zip}")

    try:
        with zipfile.ZipFile(out_zip, "w", compression=zipfile.ZIP_DEFLATED) as z:
            for fp, arc in deduped:
                try:
                    z.write(fp, arcname=arc)
                except Exception as e:
                    msg = f"[WARN] Failed to add {fp} -> {arc}: {e}"
                    print_and_log(log_lines, msg)

            z.writestr("SOTS_PluginSourceBundle_manifest.json", json.dumps(manifest, indent=2))

            report_lines: List[str] = []
            report_lines.append("SOTS Plugin Source Bundle Report")
            report_lines.append(f"timestamp: {manifest['timestamp']}")
            report_lines.append(f"project_root: {manifest['project_root']}")
            report_lines.append(f"included_plugins: {len(manifest['included_plugins'])}")
            report_lines.append(f"files: {manifest['file_count']}")
            report_lines.append(f"approx_input_bytes: {manifest['approx_input_bytes']:,}")
            report_lines.append("")
            report_lines.append("Warnings:")
            report_lines.extend(warnings or ["(none)"])
            report_lines.append("")
            report_lines.append("Included plugin folders:")
            for p in included:
                report_lines.append(f"- {p.folder.relative_to(project_root)}")
            z.writestr("SOTS_PluginSourceBundle_report.txt", "\n".join(report_lines))

    except Exception as e:
        print_and_log(log_lines, f"[ERROR] Failed to write zip: {e}")
        return 3

    print_and_log(log_lines, "[OK] Zip bundle created.")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Zip ONLY .uplugin + Source/ for SOTS suite plugins into one zip (ignores all other plugins)."
    )
    ap.add_argument(
        "--root",
        type=str,
        default="",
        help="Project root. If omitted, auto-detected by walking up to find Plugins/.",
    )
    ap.add_argument(
        "--output",
        type=str,
        default="",
        help="Output zip path. Default: <ProjectRoot>/DevTools/out/SOTS_PluginSourceBundle_<timestamp>.zip",
    )
    ap.add_argument(
        "--extra-plugins",
        type=str,
        default="",
        help="Comma-separated extra plugin folder names or .uplugin stems to include (optional).",
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="Don’t write a zip; just print what would be included.",
    )
    args = ap.parse_args()

    start = Path(args.root) if args.root.strip() else Path.cwd()
    project_root = find_project_root(start)

    log_lines: List[str] = []
    print_and_log(log_lines, f"[INFO] StartPath  : {start.resolve()}")
    print_and_log(log_lines, f"[INFO] ProjectRoot: {project_root}")

    plugins_dir = project_root / "Plugins"
    if not plugins_dir.is_dir():
        print_and_log(log_lines, "[ERROR] Plugins/ directory not found. Check --root.")
        out_log = Path.cwd() / f"SOTS_PluginSourceBundle_{now_stamp()}.log"
        out_log.write_text("\n".join(log_lines) + "\n", encoding="utf-8")
        print(f"[INFO] Wrote log: {out_log}")
        return 2

    # Canonical suite allowlist (prefer laws, fallback to hardcoded)
    canonical_suite: Set[str] = set()
    laws_path = find_laws_file(project_root)
    if laws_path:
        parsed = parse_canonical_suite_from_laws(laws_path)
        if parsed:
            canonical_suite = parsed
            print_and_log(log_lines, f"[INFO] Canonical suite parsed from laws: {laws_path}")
            print_and_log(log_lines, f"[INFO] Canonical suite count: {len(canonical_suite)}")
        else:
            print_and_log(log_lines, f"[WARN] Laws file found but suite list not parsed: {laws_path}")

    if not canonical_suite:
        canonical_suite = set(FALLBACK_CANONICAL_SUITE)
        print_and_log(log_lines, "[WARN] Using fallback canonical suite allowlist (laws parse unavailable).")
        print_and_log(log_lines, f"[INFO] Fallback suite count: {len(canonical_suite)}")

    extra_allow: Set[str] = set()
    if args.extra_plugins.strip():
        extra_allow = {x.strip() for x in args.extra_plugins.split(",") if x.strip()}
        print_and_log(log_lines, f"[INFO] Extra allowlist: {sorted(extra_allow)}")

    ts = now_stamp()
    default_out = project_root / "DevTools" / "out" / f"SOTS_PluginSourceBundle_{ts}.zip"
    out_zip = Path(args.output).expanduser() if args.output.strip() else default_out
    if not out_zip.is_absolute():
        out_zip = (project_root / out_zip).resolve()

    out_log = out_zip.with_suffix(".log")
    rc = build_zip_bundle(
        project_root=project_root,
        out_zip=out_zip,
        canonical_suite=canonical_suite,
        extra_allow=extra_allow,
        dry_run=bool(args.dry_run),
        log_lines=log_lines,
    )

    out_log.parent.mkdir(parents=True, exist_ok=True)
    out_log.write_text("\n".join(log_lines) + "\n", encoding="utf-8")
    print(f"[INFO] Wrote log: {out_log}")

    if rc == 0 and not args.dry_run and out_zip.exists():
        print(f"[INFO] Output zip size: {out_zip.stat().st_size:,} bytes")

    return rc


if __name__ == "__main__":
    raise SystemExit(main())
