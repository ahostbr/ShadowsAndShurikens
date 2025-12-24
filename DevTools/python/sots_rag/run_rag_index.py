from __future__ import annotations

import argparse
from pathlib import Path

from rag_indexer import RAGIndexer

LOGICAL_TRUE = {"1", "true", "yes", "y", "on"}
LOGICAL_FALSE = {"0", "false", "no", "n", "off"}


def parse_bool(value: str | None, default: bool) -> bool:
    if value is None:
        return default
    normalized = value.strip().lower()
    if normalized in LOGICAL_TRUE:
        return True
    if normalized in LOGICAL_FALSE:
        return False
    raise argparse.ArgumentTypeError(f"Invalid boolean value: '{value}'")


def detect_project_root(start: Path | None = None) -> Path:
    path = start if start and start.is_dir() else (start or Path(__file__).resolve()).parent
    for candidate in [path] + list(path.parents):
        if (candidate / "Plugins").is_dir() and (candidate / "DevTools" / "python").is_dir():
            return candidate
    raise RuntimeError("Could not auto-detect project root.")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Build the sots_rag index for the SAS repo.")
    parser.add_argument("--project_root", help="Project root path")
    parser.add_argument("--reports_dir", help="Reports output directory")
    parser.add_argument("--include_docs", default="true", help="Include docs (.md/.txt) (true/false)")
    parser.add_argument("--full", action="store_true", help="Rebuild all chunks")
    parser.add_argument("--changed_only", default="true", help="Only process changed files (true/false)")
    parser.add_argument("--plugin_filter", default="", help="Optional plugin name glob(s), comma-separated")
    parser.add_argument(
        "--embedding_backend", default="sentence_transformers", help="Embedding backend (sentence_transformers|hash)"
    )
    parser.add_argument("--embedding_model", help="Optional embedding model name")
    parser.add_argument("--verbose", action="store_true")

    args = parser.parse_args(argv)
    project_root = Path(args.project_root).resolve() if args.project_root else detect_project_root(Path(__file__).resolve())
    reports_dir = Path(args.reports_dir).resolve() if args.reports_dir else project_root / "Reports" / "RAG"
    include_docs = parse_bool(args.include_docs, True)
    changed_only = parse_bool(args.changed_only, True)

    indexer = RAGIndexer(
        project_root=project_root,
        reports_dir=reports_dir,
        include_docs=include_docs,
        full=args.full,
        changed_only=changed_only,
        plugin_filter=args.plugin_filter,
        embedding_backend=args.embedding_backend,
        embedding_model=args.embedding_model,
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
