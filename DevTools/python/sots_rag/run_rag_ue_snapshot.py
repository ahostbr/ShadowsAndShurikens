from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
import sys
from pathlib import Path
from typing import Dict

from rag_indexer import RAGIndexer
from run_rag_index import detect_project_root, parse_bool

VALID_MODES = {"runtime", "editor", "both"}


def _timestamp() -> str:
    return dt.datetime.now().strftime("%Y%m%d_%H%M%S")


def _ensure_missing(path: Path, label: str) -> None:
    if path.exists():
        raise RuntimeError(f"{label} already exists: {path}")


def _build_stage_links(ue_root: Path, mode: str) -> Dict[str, Path]:
    runtime = ue_root / "Engine" / "Source" / "Runtime"
    developer = ue_root / "Engine" / "Source" / "Developer"
    if not runtime.is_dir():
        raise RuntimeError(f"Missing UE Runtime source folder: {runtime}")
    if not developer.is_dir():
        raise RuntimeError(f"Missing UE Developer source folder: {developer}")
    links = {"Runtime": runtime, "Developer": developer}
    if mode in {"editor", "both"}:
        editor = ue_root / "Engine" / "Source" / "Editor"
        if not editor.is_dir():
            raise RuntimeError(f"Missing UE Editor source folder: {editor}")
        links["Editor"] = editor
    return links


def _create_junction(link_path: Path, target: Path) -> None:
    if link_path.exists():
        raise RuntimeError(f"Stage link already exists: {link_path}")
    if not target.is_dir():
        raise RuntimeError(f"Target directory missing: {target}")
    result = subprocess.run(
        ["cmd", "/c", "mklink", "/J", str(link_path), str(target)],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        combined = "; ".join(s for s in [result.stdout.strip(), result.stderr.strip()] if s)
        raise RuntimeError(f"mklink failed for {link_path}: {combined or 'unknown error'}")
    if not link_path.exists():
        raise RuntimeError(f"Junction was not created: {link_path}")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Build a one-time UE Engine RAG snapshot.")
    parser.add_argument("--ue_root", required=True, help="UE install root (folder containing Engine/)")
    parser.add_argument("--mode", default="runtime", help="runtime|editor|both")
    parser.add_argument("--out_base", help="Snapshot base folder (default <SAS_ROOT>/Reports/RAG_UE_Snapshots)")
    parser.add_argument("--name", help="Snapshot folder name override")
    parser.add_argument("--embedding_backend", default="hash", help="Embedding backend (hash|sentence_transformers)")
    parser.add_argument("--include_docs", default="false", help="Include docs (.md/.txt) (true/false)")
    parser.add_argument("--verbose", action="store_true")

    args = parser.parse_args(argv)

    try:
        mode = args.mode.strip().lower()
        if mode not in VALID_MODES:
            raise RuntimeError(f"Invalid mode '{args.mode}'. Use runtime|editor|both.")

        ue_root = Path(args.ue_root).resolve()
        if not (ue_root / "Engine").is_dir():
            raise RuntimeError(f"UE root must contain Engine/: {ue_root}")

        project_root = detect_project_root(Path(__file__).resolve())
        out_base = Path(args.out_base).resolve() if args.out_base else project_root / "Reports" / "RAG_UE_Snapshots"
        out_base.mkdir(parents=True, exist_ok=True)

        timestamp = _timestamp()
        snapshot_name = args.name or f"{ue_root.name}_{mode}_{timestamp}"
        snapshot_dir = out_base / snapshot_name
        _ensure_missing(snapshot_dir, "Snapshot folder")
        snapshot_dir.mkdir(parents=True, exist_ok=False)

        stage_dir = snapshot_dir / "_stage"
        rag_dir = snapshot_dir / "RAG"
        stage_dir.mkdir(parents=True, exist_ok=False)
        rag_dir.mkdir(parents=True, exist_ok=False)

        stage_links = _build_stage_links(ue_root, mode)
        print(f"[ue_snapshot] Snapshot: {snapshot_dir}")
        print(f"[ue_snapshot] Stage root: {stage_dir}")
        for name, target in stage_links.items():
            link_path = stage_dir / name
            print(f"[ue_snapshot] Linking {link_path} -> {target}")
            _create_junction(link_path, target)

        include_docs = parse_bool(args.include_docs, False)

        indexer = RAGIndexer(
            project_root=stage_dir,
            reports_dir=rag_dir,
            include_docs=include_docs,
            full=True,
            changed_only=False,
            plugin_filter="",
            embedding_backend=args.embedding_backend,
            embedding_model=None,
            verbose=args.verbose,
        )
        summary = indexer.run()

        print("Index summary:")
        print(f"  Files scanned: {summary['files_scanned']}")
        print(f"  Files changed: {summary['files_changed']} (+{summary['files_added']} new)")
        print(f"  Files deleted: {summary['files_deleted']}")
        print(f"  Chunks total: {summary['chunks_total']} (+{summary['chunks_added']} new)")
        print(f"  Embeddings computed: {summary['embeddings']}")
        print(f"  Elapsed: {summary['elapsed']:.2f}s")

        meta_path = snapshot_dir / "snapshot_meta.json"
        meta = {
            "timestamp": timestamp,
            "ue_root": str(ue_root),
            "mode": mode,
            "include_docs": include_docs,
            "embedding_backend": args.embedding_backend,
            "snapshot_dir": str(snapshot_dir),
            "stage_dir": str(stage_dir),
            "rag_dir": str(rag_dir),
            "out_base": str(out_base),
            "stage_links": {name: str(path) for name, path in stage_links.items()},
        }
        meta_path.write_text(json.dumps(meta, indent=2, sort_keys=True), encoding="utf-8")
        print(f"[ue_snapshot] Wrote metadata: {meta_path}")
    except Exception as exc:
        print(f"[ue_snapshot] ERROR: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
