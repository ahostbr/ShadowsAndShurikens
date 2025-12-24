from __future__ import annotations

import datetime as dt
import json
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

from bm25 import BM25Index, tokenize
from embeddings import EmbeddingProvider
from repoindex_bridge import RepoIndexBridge
from schemas import (
    BM25_FILE,
    CHUNKS_FILE,
    VECTOR_FILE,
)
from vector_store import VectorStore


def detect_project_root(start: Optional[Path] = None) -> Path:
    path = start if start and start.is_dir() else (start or Path(__file__).resolve()).parent
    for parent in [path] + list(path.parents):
        if (parent / "Plugins").is_dir() and (parent / "DevTools" / "python").is_dir():
            return parent
    raise RuntimeError("Could not auto-detect project root.")


class RAGQuery:
    EXACT_WEIGHT = 3.0
    BM25_WEIGHT = 1.0
    VECTOR_WEIGHT = 0.5

    def __init__(
        self,
        project_root: Path,
        reports_dir: Path,
        embedding_backend: str = "sentence_transformers",
        embedding_model: Optional[str] = None,
        verbose: bool = False,
    ) -> None:
        self.project_root = project_root.resolve()
        self.reports_dir = reports_dir.resolve()
        self.log_lines: List[str] = []
        self.verbose = verbose
        self.embedding_provider = EmbeddingProvider(
            backend=embedding_backend,
            model_name=embedding_model,
            logger=self._log,
        )
        self.chunks = self._load_chunks()
        if not self.chunks:
            raise RuntimeError("Chunk index not found (run sots_rag_index first).")
        self.chunks_by_path = self._build_chunks_by_path()
        self.bm25 = self._load_bm25()
        self.vector_store = self._load_vector_store()
        self.bridge = RepoIndexBridge(self.project_root)
        self.bridge.load()
        self._log(f"Embedding backend: {self.embedding_provider.get_model_name()}")

    def _log(self, msg: str) -> None:
        line = f"[rag_query] {msg}"
        print(line)
        self.log_lines.append(f"{dt.datetime.now().isoformat(timespec='seconds')} {msg}")

    def _load_chunks(self) -> Dict[str, Dict[str, object]]:
        path = self.reports_dir / CHUNKS_FILE
        chunks: Dict[str, Dict[str, object]] = {}
        if not path.exists():
            return {}
        with path.open("r", encoding="utf-8") as fh:
            for line in fh:
                line = line.strip()
                if not line:
                    continue
                chunk = json.loads(line)
                cid = chunk.get("chunk_id")
                if cid:
                    chunks[cid] = chunk
        return chunks

    def _load_bm25(self) -> BM25Index:
        path = self.reports_dir / BM25_FILE
        if not path.exists():
            return BM25Index()
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
            return BM25Index.from_dict(data)
        except Exception:
            return BM25Index()

    def _load_vector_store(self) -> VectorStore:
        path = self.reports_dir / VECTOR_FILE
        if not path.exists():
            return VectorStore()
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
            return VectorStore.from_dict(data)
        except Exception:
            return VectorStore()

    def _build_chunks_by_path(self) -> Dict[str, List[Dict[str, object]]]:
        grouped: Dict[str, List[Dict[str, object]]] = {}
        for chunk in self.chunks.values():
            path = chunk.get("path", "")
            grouped.setdefault(path, []).append(chunk)
        for entries in grouped.values():
            entries.sort(key=lambda c: c.get("start_line", 1))
        return grouped

    def _find_chunk_for_path_line(self, rel_path: str, line: int) -> Optional[Dict[str, object]]:
        for chunk in self.chunks_by_path.get(rel_path, []):
            start = chunk.get("start_line", 0)
            end = chunk.get("end_line", 0)
            if start <= line <= end:
                return chunk
        return None

    def _add_candidate(
        self,
        candidate_map: Dict[str, Dict[str, object]],
        chunk_id: str,
        source: str,
        score: float,
    ) -> None:
        if chunk_id not in self.chunks:
            return
        entry = candidate_map.setdefault(chunk_id, {"chunk": self.chunks[chunk_id], "score_breakdown": {}})
        entry["score_breakdown"][source] = max(entry["score_breakdown"].get(source, 0.0), score)

    def _exact_candidates(self, query: str) -> List[str]:
        matches = []
        for match in self.bridge.exact_matches(query):
            path = match.get("path", "")
            line = match.get("line")
            if not path or not isinstance(line, int):
                continue
            chunk = self._find_chunk_for_path_line(path, line)
            if chunk:
                matches.append(chunk["chunk_id"])
        return matches

    def _bm25_candidates(self, tokens: Sequence[str], top_n: int) -> List[Tuple[str, float]]:
        if not tokens:
            return []
        scores = self.bm25.score_all(tokens)
        items = sorted(scores.items(), key=lambda pair: pair[1], reverse=True)
        return items[:top_n]

    def _vector_candidates(self, query: str, top_n: int) -> List[Tuple[str, float]]:
        if not self.vector_store.get_dim():
            return []
        try:
            vector = self.embedding_provider.embed(query)
        except Exception as exc:
            self.log(f"Embedding query failed: {exc}")
            return []
        return self.vector_store.search(vector, top_k=top_n)

    def _weight(self, source: str) -> float:
        if source == "exact":
            return self.EXACT_WEIGHT
        if source == "bm25":
            return self.BM25_WEIGHT
        if source == "vector":
            return self.VECTOR_WEIGHT
        return 1.0

    def _merge_candidates(
        self,
        exact_ids: List[str],
        bm25_items: List[Tuple[str, float]],
        vector_items: List[Tuple[str, float]],
    ) -> List[Dict[str, object]]:
        candidate_map: Dict[str, Dict[str, object]] = {}
        for chunk_id in exact_ids:
            self._add_candidate(candidate_map, chunk_id, "exact", 1.0)
        for chunk_id, score in bm25_items:
            self._add_candidate(candidate_map, chunk_id, "bm25", score)
        for chunk_id, score in vector_items:
            self._add_candidate(candidate_map, chunk_id, "vector", score)
        hits: List[Dict[str, object]] = []
        for chunk_id, info in candidate_map.items():
            score_breakdown = info["score_breakdown"]
            agg_score = sum(score_breakdown[src] * self._weight(src) for src in score_breakdown)
            hits.append(
                {
                    "chunk": info["chunk"],
                    "score": agg_score,
                    "score_breakdown": score_breakdown,
                }
            )
        hits.sort(key=lambda item: item["score"], reverse=True)
        return hits

    def _format_snippet(self, chunk: Dict[str, object], query_tokens: Sequence[str]) -> str:
        text = chunk.get("text", "")
        lines = [line.strip() for line in text.splitlines() if line.strip()]
        if not lines:
            return ""
        query_lower = [token.lower() for token in query_tokens]
        for line in lines:
            lower = line.lower()
            if any(token in lower for token in query_lower):
                return line[:200]
        return lines[0][:200]

    def _rerank_candidates(
        self,
        hits: List[Dict[str, object]],
        query_tokens: Sequence[str],
        rerank_k: int,
    ) -> None:
        if rerank_k <= 0 or not hits:
            return
        top = hits[:rerank_k]
        query_lower = [token.lower() for token in query_tokens]

        def score_fn(entry: Dict[str, object]) -> float:
            chunk = entry["chunk"]
            base = entry.get("score", 0.0)
            symbol_bonus = len(chunk.get("symbol_hits", [])) * 0.2
            tag_bonus = len(chunk.get("tag_hits", [])) * 0.1
            text_lower = chunk.get("text", "").lower()
            match_bonus = sum(1 for token in query_lower if token and token in text_lower) * 0.01
            return base + symbol_bonus + tag_bonus + match_bonus

        top.sort(key=score_fn, reverse=True)
        hits[:rerank_k] = top

    def run_query(
        self,
        query: str,
        top_k: int,
        bm25_n: int,
        vec_n: int,
        rerank: bool,
        rerank_k: int,
    ) -> Dict[str, object]:
        tokens = tokenize(query)
        exact_matches = self._exact_candidates(query)
        bm25_items = self._bm25_candidates(tokens, bm25_n)
        vector_items = self._vector_candidates(query, vec_n)
        hits = self._merge_candidates(exact_matches, bm25_items, vector_items)
        if rerank:
            self._rerank_candidates(hits, tokens, rerank_k)
        timestamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
        top_hits = hits[:top_k]
        formatted_hits: List[Dict[str, object]] = []
        for idx, hit in enumerate(top_hits, start=1):
            chunk = hit["chunk"]
            path = chunk.get("path", "")
            start_line = chunk.get("start_line", 0)
            end_line = chunk.get("end_line", 0)
            snippet = self._format_snippet(chunk, tokens)
            formatted_hits.append(
                {
                    "rank": idx,
                    "chunk_id": chunk.get("chunk_id"),
                    "path": path,
                    "plugin": chunk.get("plugin"),
                    "module": chunk.get("module"),
                    "kind": chunk.get("kind"),
                    "start_line": start_line,
                    "end_line": end_line,
                    "score": hit["score"],
                    "score_breakdown": hit["score_breakdown"],
                    "snippet": snippet,
                    "how_to_open": f"{path}:{start_line}-{end_line}",
                    "symbol_hits": chunk.get("symbol_hits", []),
                    "tag_hits": chunk.get("tag_hits", []),
                }
            )
        return {
            "timestamp": timestamp,
            "query": query,
            "repo_index_available": self.bridge.has_repo_index(),
            "exact_hits": len(exact_matches),
            "bm25_candidates": len(bm25_items),
            "vector_candidates": len(vector_items),
            "top_hits": formatted_hits,
        }
