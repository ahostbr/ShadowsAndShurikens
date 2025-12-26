from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List

from schemas import MANIFEST_FILE


def _load_manifest(snapshot_dir: Path) -> Dict[str, Dict[str, object]]:
    manifest_path = snapshot_dir / "RAG" / MANIFEST_FILE
    if not manifest_path.exists():
        raise RuntimeError(f"Missing manifest: {manifest_path}")
    payload = json.loads(manifest_path.read_text(encoding="utf-8"))
    files = payload.get("files", {})
    if not isinstance(files, dict):
        raise RuntimeError(f"Invalid manifest format in {manifest_path}")
    return files


def _format_section(title: str, items: List[str]) -> List[str]:
    lines = [f"{title} ({len(items)}):"]
    if items:
        lines.extend(f"  {path}" for path in items)
    else:
        lines.append("  (none)")
    lines.append("")
    return lines


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Diff two UE Engine RAG snapshots.")
    parser.add_argument("--old", required=True, help="Old snapshot folder")
    parser.add_argument("--new", required=True, help="New snapshot folder")
    parser.add_argument("--out", help="Text report path (default <new>/diff_from_<old>.txt)")
    parser.add_argument("--json", dest="json_out", help="JSON report path (default <new>/diff_from_<old>.json)")

    args = parser.parse_args(argv)

    try:
        old_snapshot = Path(args.old).resolve()
        new_snapshot = Path(args.new).resolve()

        old_files = _load_manifest(old_snapshot)
        new_files = _load_manifest(new_snapshot)

        old_keys = set(old_files.keys())
        new_keys = set(new_files.keys())

        added = sorted(new_keys - old_keys)
        removed = sorted(old_keys - new_keys)
        changed = sorted(
            key
            for key in old_keys & new_keys
            if old_files.get(key, {}).get("content_hash") != new_files.get(key, {}).get("content_hash")
        )

        old_base = old_snapshot.name
        out_txt = Path(args.out).resolve() if args.out else new_snapshot / f"diff_from_{old_base}.txt"
        out_json = Path(args.json_out).resolve() if args.json_out else new_snapshot / f"diff_from_{old_base}.json"

        counts = {"added": len(added), "removed": len(removed), "changed": len(changed)}

        report_lines = [
            "UE RAG Snapshot Diff",
            f"Old snapshot: {old_snapshot}",
            f"New snapshot: {new_snapshot}",
            "",
            f"Counts: added={counts['added']} removed={counts['removed']} changed={counts['changed']}",
            "",
        ]
        report_lines.extend(_format_section("Added", added))
        report_lines.extend(_format_section("Removed", removed))
        report_lines.extend(_format_section("Changed", changed))
        out_txt.write_text("\n".join(report_lines), encoding="utf-8")

        payload = {"added": added, "removed": removed, "changed": changed, "counts": counts}
        out_json.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")

        print("[ue_snapshot_diff] Diff complete.")
        print(f"[ue_snapshot_diff] Added: {counts['added']}")
        print(f"[ue_snapshot_diff] Removed: {counts['removed']}")
        print(f"[ue_snapshot_diff] Changed: {counts['changed']}")
        print(f"[ue_snapshot_diff] Wrote: {out_txt}")
        print(f"[ue_snapshot_diff] Wrote: {out_json}")
    except Exception as exc:
        print(f"[ue_snapshot_diff] ERROR: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
