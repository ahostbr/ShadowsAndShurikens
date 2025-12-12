"""
Helpers for discovering BPGen snippet packs on disk.

- No Unreal dependencies; pure filesystem + JSON metadata.
- Always prints what it discovers or errors on.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, List, Optional


def get_devtools_root() -> Path:
    """
    Returns the DevTools root (parent of this python folder).
    """
    return Path(__file__).resolve().parent.parent


def get_default_packs_root() -> Path:
    """
    Default packs root: <DevTools>/bpgen_snippets/packs
    """
    return get_devtools_root() / "bpgen_snippets" / "packs"


def load_pack_meta(pack_path: Path | str) -> Dict[str, Any]:
    """
    Load pack_meta.json from a pack folder. Adds 'path' key.
    On error, prints and returns {}.
    """
    pack_dir = Path(pack_path).resolve()
    meta_path = pack_dir / "pack_meta.json"
    if not meta_path.exists():
        print(f"[WARN] pack_meta.json not found in {pack_dir}")
        return {}

    try:
        with meta_path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as exc:
        print(f"[ERROR] Failed to read {meta_path}: {exc}")
        return {}

    if not isinstance(data, dict):
        print(f"[ERROR] Invalid pack_meta format (expected object): {meta_path}")
        return {}

    data["path"] = str(pack_dir)
    return data


def discover_packs(root: Path | str | None = None) -> List[Dict[str, Any]]:
    """
    Discover packs under the given root (default: bpgen_snippets/packs).
    Returns list of pack_meta dicts with 'path'.
    """
    packs_root = Path(root) if root else get_default_packs_root()
    packs_root = packs_root.resolve()
    packs: List[Dict[str, Any]] = []

    print(f"[INFO] Discovering BPGen packs in: {packs_root}")
    if not packs_root.is_dir():
        print(f"[WARN] Packs root does not exist: {packs_root}")
        return packs

    for child in sorted(packs_root.iterdir()):
        if not child.is_dir():
            continue
        meta = load_pack_meta(child)
        if not meta:
            continue
        packs.append(meta)
        pack_name = meta.get("pack_name", child.name)
        version = meta.get("version", "unknown")
        snippet_count = len(meta.get("snippets", [])) if isinstance(meta.get("snippets"), list) else "?"
        print(f"[INFO] Found pack '{pack_name}' v{version} (snippets: {snippet_count}) at {child}")

    if not packs:
        print("[WARN] No packs discovered.")
    else:
        print(f"[INFO] Discovered {len(packs)} pack(s).")

    return packs


def discover_snippets_in_pack(pack_path: Path | str) -> List[Dict[str, Any]]:
    """
    Enumerate snippets in a pack by matching *_Job.json and *_GraphSpec.json.
    Returns list of dicts with snippet_name, job_path, graphspec_path.
    """
    pack_dir = Path(pack_path).resolve()
    jobs_dir = pack_dir / "jobs"
    graphs_dir = pack_dir / "graphspecs"
    snippets: List[Dict[str, Any]] = []

    if not jobs_dir.exists():
        print(f"[WARN] jobs/ folder missing in pack: {pack_dir}")
        return snippets

    job_files = sorted(jobs_dir.glob("*_Job.json"))
    if not job_files:
        print(f"[INFO] No *_Job.json files found under {jobs_dir}")
        return snippets

    for job_file in job_files:
        snippet_name = job_file.stem[:-len("_Job")] if job_file.stem.endswith("_Job") else job_file.stem
        graph_file = graphs_dir / f"{snippet_name}_GraphSpec.json"
        graph_path: Optional[str]
        if graph_file.exists():
            graph_path = str(graph_file)
        else:
            graph_path = None
            print(f"[WARN] GraphSpec missing for snippet '{snippet_name}' (expected {graph_file})")

        snippets.append(
            {
                "snippet_name": snippet_name,
                "job_path": str(job_file),
                "graphspec_path": graph_path,
            }
        )

    print(f"[INFO] Discovered {len(snippets)} snippet(s) in pack: {pack_dir}")
    return snippets


if __name__ == "__main__":
    # Simple smoke test when run directly.
    packs = discover_packs()
    for pack in packs:
        discover_snippets_in_pack(pack.get("path", ""))
