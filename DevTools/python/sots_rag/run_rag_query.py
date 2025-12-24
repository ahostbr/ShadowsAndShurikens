from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

from rag_query import RAGQuery, detect_project_root

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


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Query the sots_rag index for the SAS repo.")
    parser.add_argument("--project_root", help="Project root path")
    parser.add_argument("--reports_dir", help="Reports directory (default <root>/Reports/RAG)")
    parser.add_argument("--q", "--query", dest="query", help="Query string")
    parser.add_argument("--top_k", type=int, default=12, help="Number of final hits to return")
    parser.add_argument("--bm25_n", type=int, default=20, help="BM25 candidates to consider")
    parser.add_argument("--vec_n", type=int, default=20, help="Vector candidates to consider")
    parser.add_argument("--rerank", default="false", help="Enable heuristic reranking (true/false)")
    parser.add_argument("--rerank_k", type=int, default=30, help="Number of top hits to rerank")
    parser.add_argument("--embedding_backend", default="sentence_transformers", help="Embedding backend")
    parser.add_argument("--embedding_model", help="Optional embedding model name")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("positional_query", nargs="*", help="Query if --q not supplied")

    args = parser.parse_args(argv)
    query_text = args.query.strip() if args.query else " ".join(args.positional_query).strip()
    if not query_text:
        parser.error("A query must be provided via --q or positional args.")

    project_root = Path(args.project_root).resolve() if args.project_root else detect_project_root(Path(__file__).resolve())
    reports_dir = Path(args.reports_dir).resolve() if args.reports_dir else project_root / "Reports" / "RAG"
    reports_dir.mkdir(parents=True, exist_ok=True)

    rerank = parse_bool(args.rerank, False)

    query_engine = RAGQuery(
        project_root=project_root,
        reports_dir=reports_dir,
        embedding_backend=args.embedding_backend,
        embedding_model=args.embedding_model,
        verbose=args.verbose,
    )
    result = query_engine.run_query(
        query=query_text,
        top_k=args.top_k,
        bm25_n=args.bm25_n,
        vec_n=args.vec_n,
        rerank=rerank,
        rerank_k=args.rerank_k,
    )

    print(f"RepoIndex available: {result['repo_index_available']}, exact hits: {result['exact_hits']}")
    print("Top hits summary:")
    for hit in result["top_hits"]:
        display_path = hit["path"].replace("/", os.sep)
        print(f"  {hit['rank']}. {display_path} ({hit['start_line']}-{hit['end_line']}) score={hit['score']:.3f}")

    timestamp = result["timestamp"]
    report_path = reports_dir / f"rag_query_{timestamp}.txt"
    json_path = reports_dir / f"rag_query_{timestamp}.json"
    log_path = reports_dir / f"rag_query_{timestamp}.log"

    report_lines = [
        "RAG Query Report",
        f"Query: {query_text}",
        f"Timestamp: {timestamp}",
        f"RepoIndex available: {result['repo_index_available']}",
        "",
        "Top Hits:",
    ]
    for hit in result["top_hits"]:
        report_lines.append(
            f"{hit['rank']}. {hit['path']} ({hit['start_line']}-{hit['end_line']}) score={hit['score']:.3f}"
        )
        if hit["snippet"]:
            report_lines.append(f"    Snippet: {hit['snippet']}")
        report_lines.append(f"    How to open: {hit['how_to_open']}")

    report_path.write_text("\n".join(report_lines), encoding="utf-8")

    report_payload = {
        "query": result["query"],
        "timestamp": timestamp,
        "repo_index_available": result["repo_index_available"],
        "stats": {
            "exact_hits": result["exact_hits"],
            "bm25_candidates": result["bm25_candidates"],
            "vector_candidates": result["vector_candidates"],
        },
        "top_hits": result["top_hits"],
    }
    json_path.write_text(json.dumps(report_payload, indent=2, ensure_ascii=False), encoding="utf-8")

    log_lines = [
        f"[rag_query] Query: {query_text}",
        f"[rag_query] RepoIndex available: {result['repo_index_available']}",
        f"[rag_query] Exact hits: {result['exact_hits']}",
        f"[rag_query] BM25 candidates: {result['bm25_candidates']}",
        f"[rag_query] Vector candidates: {result['vector_candidates']}",
        "[rag_query] Top hits:",
    ]
    for hit in result["top_hits"]:
        log_lines.append(
            f"[rag_query] {hit['rank']}. {hit['path']} ({hit['start_line']}-{hit['end_line']}) score={hit['score']:.3f}"
        )
    log_lines.extend(query_engine.log_lines)
    log_path.write_text("\n".join(log_lines), encoding="utf-8")

    print(f"Reports: {report_path}, {json_path}, log: {log_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
