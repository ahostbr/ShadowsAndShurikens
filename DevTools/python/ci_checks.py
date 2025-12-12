#!/usr/bin/env python3
"""
Non-interactive CI check runner for DevTools plugin verification.
Runs a lightweight plugin audit and Tag Spine heuristic scanner, then exits with a non-zero
status code if issues were found.
"""
import os
import re
import sys
from pathlib import Path

from hud_widget_usage_check import find_violations as find_hud_widget_violations


ROOT = Path(__file__).resolve().parents[2]
PLUGINS = ROOT / "Plugins"

def audit_plugins():
    results = []
    for plugin_dir in sorted(p for p in PLUGINS.iterdir() if p.is_dir()):
        name = plugin_dir.name
        has_bins = (plugin_dir / "Binaries").exists()
        has_intermediate = (plugin_dir / "Intermediate").exists()
        results.append((name, has_bins, has_intermediate))
    return results


def scan_tag_spine():
    pattern = re.compile(r"\b(\.AddTag|\.RemoveTag)\s*\(")
    whitelist = {"SOTS_TagManager"}
    matches = []
    for dirpath, dirnames, filenames in os.walk(PLUGINS):
        rel = os.path.relpath(dirpath, PLUGINS)
        parts = rel.split(os.sep)
        if parts[0] in whitelist:
            continue
        for fname in filenames:
            if not fname.endswith((".cpp", ".cc", ".h", ".hpp")):
                continue
            fpath = Path(dirpath) / fname
            try:
                with open(fpath, 'r', encoding='utf-8', errors='ignore') as fh:
                    for i, line in enumerate(fh, start=1):
                        if pattern.search(line):
                            matches.append((fpath, i, line.strip()))
            except Exception:
                continue
    return matches


def main():
    print("Running CI checks: plugin audit + Tag Spine scanner")

    audit = audit_plugins()
    print(f"[INFO] Plugin count: {len(audit)}")
    with_bins = [p for p in audit if p[1]]
    with_inter = [p for p in audit if p[2]]
    print(f"[INFO] Plugins with bins: {len(with_bins)}; Intermediate: {len(with_inter)}")

    matches = scan_tag_spine()
    if matches:
        print("[ERROR] Suspicious AddTag/RemoveTag usage found outside SOTS_TagManager:")
        for fpath, lineno, line in matches:
            print(f" - {fpath.relative_to(ROOT)}:{lineno} -> {line}")
        sys.exit(1)

    hud_violations = find_hud_widget_violations()
    if hud_violations:
        print("[ERROR] HUD widget subsystem usage found outside Plugins/SOTS_UI:")
        for fpath, lineno, label, line in hud_violations:
            print(f" - {fpath.relative_to(ROOT)}:{lineno} ({label}) -> {line}")
        sys.exit(1)

    print("[OK] No suspicious AddTag/RemoveTag usage found outside SOTS_TagManager.")
    print("[OK] HUD and notification subsystems are touched only by Plugins/SOTS_UI.")
    print("CI checks passed.")
    sys.exit(0)


if __name__ == '__main__':
    main()
