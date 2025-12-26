from __future__ import annotations

import datetime as dt
import fnmatch
import hashlib
import json
import os
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple

from bm25 import BM25Index, tokenize
from embeddings import EmbeddingProvider
from repoindex_bridge import RepoIndexBridge
from schemas import (
    BM25_FILE,
    CHUNKS_FILE,
    MANIFEST_FILE,
    REPORT_FILE,
    RUN_LOG_FILE,
    SCHEMA_VERSION,
    VECTOR_FILE,
    ChunkRecord,
)
from vector_store import VectorStore

CODE_EXTS = {".h", ".hpp", ".cpp", ".inl"}
CONFIG_EXTS = {".cs", ".uplugin", ".ini"}
DOC_EXTS = {".md", ".txt"}

SKIP_DIRS = {
    "binaries",
    "intermediate",
    "saved",
    "deriveddatacache",
    ".git",
    ".vs",
    "node_modules",
    "reports",
    "devtools",
}

CHUNK_SETTINGS = {
    "code": {"size": 300, "overlap": 40},
    "config": {"size": 250, "overlap": 30},
    "doc": {"size": 600, "overlap": 80},
}


def _sha1_file(path: Path) -> str:
    h = hashlib.sha1()
    with path.open("rb") as fh:
        while chunk := fh.read(1024 * 1024):
            h.update(chunk)
    return h.hexdigest()


def _normalize_rel_path(project_root: Path, path: Path) -> str:
    try:
        return path.relative_to(project_root).as_posix()
    except ValueError:
        return path.resolve().relative_to(project_root.resolve()).as_posix()


def _extract_plugin_name(rel_path: str) -> str:
    parts = rel_path.split("/")
    if "Plugins" in parts:
        idx = parts.index("Plugins")
        if idx + 1 < len(parts):
            return parts[idx + 1]
    return ""


class RAGIndexer:
    def __init__(
        self,
        project_root: Path,
        reports_dir: Path,
        include_code: bool = True,
        include_docs: bool = True,
        full: bool = False,
        changed_only: bool = True,
        plugin_filter: str = "",
        embedding_backend: str = "sentence_transformers",
        embedding_model: Optional[str] = None,
        verbose: bool = False,
    ) -> None:
        self.project_root = project_root.resolve()
        self.reports_dir = reports_dir.resolve()
        self.tool_root = Path(__file__).resolve().parent
        self.cache_dir = self.tool_root / "_cache"
        self.logs_dir = self.tool_root / "_logs"
        self.manifest_path = self.reports_dir / MANIFEST_FILE
        self.chunks_path = self.reports_dir / CHUNKS_FILE
        self.bm25_path = self.reports_dir / BM25_FILE
        self.vector_path = self.reports_dir / VECTOR_FILE
        self.report_path = self.reports_dir / REPORT_FILE
        self.run_log_path = self.reports_dir / RUN_LOG_FILE
        self.include_code = bool(include_code)
        self.include_docs = bool(include_docs)
        self.full = bool(full)
        self.changed_only = bool(changed_only)
        self.plugin_filters = [p.strip() for p in plugin_filter.split(",") if p.strip()]
        self.embedding_backend = embedding_backend
        self.embedding_model = embedding_model
        self.verbose = bool(verbose)
        self.log_lines: List[str] = []
        self._cache_file = self.cache_dir / "rag_index_cache.json"

    def log(self, message: str) -> None:
        line = f"[rag_index] {message}"
        print(line)
        ts = dt.datetime.now().isoformat(timespec="seconds")
        self.log_lines.append(f"{ts} {message}")

    def _ensure_dirs(self) -> None:
        self.reports_dir.mkdir(parents=True, exist_ok=True)
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self.logs_dir.mkdir(parents=True, exist_ok=True)

    def _load_manifest(self) -> Dict[str, Dict[str, object]]:
        if not self.manifest_path.exists() or self.full:
            return {}
        try:
            payload = json.loads(self.manifest_path.read_text(encoding="utf-8"))
            files = payload.get("files", {})
            if isinstance(files, dict):
                return files
        except Exception:
            pass
        return {}

    def _save_manifest(self, manifest: Dict[str, Dict[str, object]]) -> None:
        payload = {"schema_version": SCHEMA_VERSION, "files": manifest}
        self.manifest_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")

    def _load_chunks(self) -> Dict[str, Dict[str, object]]:
        if not self.chunks_path.exists() or self.full:
            return {}
        data: Dict[str, Dict[str, object]] = {}
        try:
            with self.chunks_path.open("r", encoding="utf-8") as fh:
                for line in fh:
                    if not line.strip():
                        continue
                    entry = json.loads(line)
                    chunk_id = entry.get("chunk_id")
                    if chunk_id:
                        data[chunk_id] = entry
        except Exception:
            return {}
        return data

    def _save_chunks(self, chunk_map: Dict[str, Dict[str, object]]) -> None:
        with self.chunks_path.open("w", encoding="utf-8") as fh:
            for chunk_id in sorted(chunk_map.keys()):
                fh.write(json.dumps(chunk_map[chunk_id], ensure_ascii=False) + "\n")

    def _load_bm25(self) -> BM25Index:
        if self.full or not self.bm25_path.exists():
            return BM25Index()
        try:
            data = json.loads(self.bm25_path.read_text(encoding="utf-8"))
            return BM25Index.from_dict(data)
        except Exception:
            return BM25Index()

    def _save_bm25(self, bm25: BM25Index) -> None:
        self.bm25_path.write_text(json.dumps(bm25.to_dict(), indent=2, sort_keys=True), encoding="utf-8")

    def _load_vector_store(self) -> VectorStore:
        if self.full or not self.vector_path.exists():
            return VectorStore()
        try:
            data = json.loads(self.vector_path.read_text(encoding="utf-8"))
            return VectorStore.from_dict(data)
        except Exception:
            return VectorStore()

    def _save_vector_store(self, store: VectorStore) -> None:
        self.vector_path.write_text(json.dumps(store.to_dict(), indent=2, sort_keys=True), encoding="utf-8")

    def _collect_files(self) -> List[Path]:
        allowed_exts: Set[str] = set()
        if self.include_code:
            allowed_exts.update(CODE_EXTS)
            allowed_exts.update(CONFIG_EXTS)
        if self.include_docs:
            allowed_exts.update(DOC_EXTS)
        if not allowed_exts:
            return []
        files: List[Path] = []
        for root, dirs, filenames in os.walk(self.project_root):
            dirs[:] = [d for d in dirs if d.lower() not in SKIP_DIRS]
            for name in filenames:
                ext = Path(name).suffix.lower()
                if ext not in allowed_exts:
                    continue
                path = Path(root) / name
                rel = _normalize_rel_path(self.project_root, path)
                if self.plugin_filters:
                    plugin_name = _extract_plugin_name(rel)
                    if not plugin_name or not any(fnmatch.fnmatch(plugin_name, pat) for pat in self.plugin_filters):
                        continue
                files.append(path)
        return sorted(files)

    def _kind_for_ext(self, ext: str) -> str:
        if ext in CODE_EXTS:
            return "code"
        if ext in CONFIG_EXTS:
            return "config"
        if ext in DOC_EXTS:
            return "doc"
        return "code"

    def _chunk_file(
        self,
        rel_path: str,
        lines: List[str],
        kind: str,
        mtime: int,
        size: int,
        bridge: RepoIndexBridge,
    ) -> List[Tuple[str, Dict[str, object]]]:
        settings = CHUNK_SETTINGS.get(kind, CHUNK_SETTINGS["code"])
        chunk_size = settings["size"]
        overlap = settings["overlap"]
        chunks: List[Tuple[str, Dict[str, object]]] = []
        offset = 0
        total = len(lines)
        if total == 0:
            lines = [""]
            total = 1
        while offset < total:
            end = min(total, offset + chunk_size)
            if end <= offset:
                break
            chunk_lines = lines[offset:end]
            text = "\n".join(chunk_lines)
            if not text.strip():
                offset = end
                if end == total:
                    break
                offset = max(0, end - overlap)
                continue
            start_line = offset + 1
            end_line = end
            chunk_hash = hashlib.sha1(text.encode("utf-8")).hexdigest()
            chunk_sig = f"{rel_path}|{start_line}|{end_line}|{chunk_hash}"
            chunk_id = hashlib.sha1(chunk_sig.encode("utf-8")).hexdigest()
            plugin, module = bridge.infer_plugin_module(rel_path)
            symbol_hits = bridge.symbols_in_range(rel_path, start_line, end_line)
            tag_hits = bridge.tags_in_range(rel_path, start_line, end_line)
            record = ChunkRecord(
                chunk_id=chunk_id,
                path=rel_path,
                plugin=plugin,
                module=module,
                kind=kind,
                start_line=start_line,
                end_line=end_line,
                text=text,
                symbol_hits=symbol_hits,
                tag_hits=tag_hits,
                mtime=mtime,
                size=size,
                content_hash=chunk_hash,
            )
            chunks.append((chunk_id, record.to_dict()))
            if end == total:
                break
            offset = max(0, end - overlap)
        return chunks

    def _remove_chunk_ids(
        self,
        chunk_ids: Iterable[str],
        chunk_map: Dict[str, Dict[str, object]],
        bm25: BM25Index,
        vectors: VectorStore,
    ) -> None:
        for chunk_id in chunk_ids:
            chunk_map.pop(chunk_id, None)
            bm25.remove_document(chunk_id)
            vectors.remove(chunk_id)

    def _write_report(
        self,
        files_scanned: int,
        files_changed: int,
        files_added: int,
        files_deleted: int,
        chunks_total: int,
        new_chunks: int,
        embeddings: int,
        repo_index_loaded: bool,
        elapsed: float,
    ) -> None:
        lines: List[str] = []
        lines.append("RAG Index Report")
        lines.append(f"Generated: {dt.datetime.now().isoformat(timespec='seconds')}")
        lines.append(f"Project root: {self.project_root}")
        lines.append(f"Include code: {self.include_code}")
        lines.append(f"Include docs: {self.include_docs}")
        if self.plugin_filters:
            lines.append(f"Plugin filter: {self.plugin_filters}")
        lines.append("")
        lines.append(f"Files scanned: {files_scanned}")
        lines.append(f"Files added: {files_added}")
        lines.append(f"Files changed: {files_changed}")
        lines.append(f"Files deleted: {files_deleted}")
        lines.append("")
        lines.append(f"Chunks stored: {chunks_total} (new: {new_chunks})")
        lines.append(f"Embeddings computed: {embeddings}")
        lines.append(f"RepoIndex data available: {repo_index_loaded}")
        lines.append(f"Elapsed (s): {elapsed:.2f}")
        self.report_path.write_text("\n".join(lines), encoding="utf-8")

    def _write_cache(self, summary: Dict[str, object]) -> None:
        self._cache_file.write_text(json.dumps(summary, indent=2, sort_keys=True), encoding="utf-8")

    def _flush_log(self) -> None:
        if not self.log_lines:
            return
        with self.run_log_path.open("a", encoding="utf-8") as fh:
            for line in self.log_lines:
                fh.write(line + "\n")
        self.log_lines.clear()

    def run(self) -> Dict[str, object]:
        start_time = dt.datetime.now()
        self._ensure_dirs()
        manifest = {} if self.full else self._load_manifest()
        chunk_map = {} if self.full else self._load_chunks()
        bm25 = BM25Index() if self.full else self._load_bm25()
        vectors = self._load_vector_store() if not self.full else VectorStore()
        embedder = EmbeddingProvider(
            backend=self.embedding_backend,
            model_name=self.embedding_model,
            logger=self.log,
        )
        self.log(f"Embedding backend: {embedder.get_model_name()}")
        if vectors.get_dim() and embedder.get_dim() and vectors.get_dim() != embedder.get_dim():
            self.log("Vector dimension changed; rebuilding vector store.")
            vectors = VectorStore()
        bridge = RepoIndexBridge(self.project_root)
        bridge.load()
        repo_index_loaded = bridge.has_repo_index() and bridge.loaded
        if repo_index_loaded:
            self.log(f"RepoIndex symbols: {len(bridge.symbols_by_name)}")
        else:
            self.log("RepoIndex data not available; skipping symbol/tag enrichment.")

        files = self._collect_files()
        files_scanned = len(files)
        seen_paths: Set[str] = set()
        changed_files: List[Tuple[str, Path, int, int]] = []
        files_added = 0

        for path in files:
            rel = _normalize_rel_path(self.project_root, path)
            seen_paths.add(rel)
            stat = path.stat()
            size = int(stat.st_size)
            mtime = int(stat.st_mtime)
            entry = manifest.get(rel)
            if entry and not self.full and self.changed_only and entry.get("size") == size and entry.get("mtime") == mtime:
                continue
            changed_files.append((rel, path, size, mtime))
            if not entry:
                files_added += 1

        deleted_paths = sorted(set(manifest.keys()) - seen_paths)
        for rel in deleted_paths:
            record = manifest.pop(rel, None)
            if record:
                self._remove_chunk_ids(record.get("chunk_ids", []), chunk_map, bm25, vectors)

        chunks_added = 0
        embeddings_computed = 0
        for rel, path, size, mtime in changed_files:
            old_record = manifest.get(rel)
            if old_record:
                self._remove_chunk_ids(old_record.get("chunk_ids", []), chunk_map, bm25, vectors)
            text = path.read_text(encoding="utf-8", errors="ignore")
            lines = text.splitlines()
            kind = self._kind_for_ext(path.suffix.lower())
            chunks = self._chunk_file(rel, lines, kind, mtime, size, bridge)
            chunk_ids: List[str] = []
            for chunk_id, chunk in chunks:
                chunk_map[chunk_id] = chunk
                tokens = tokenize(chunk["text"])
                bm25.add_document(chunk_id, tokens)
                chunk_ids.append(chunk_id)
                try:
                    vector = embedder.embed(chunk["text"])
                    vectors.add(chunk_id, vector)
                    embeddings_computed += 1
                except Exception as exc:
                    self.log(f"Embedding failed for chunk {chunk_id}: {exc}")
                    continue
            manifest[rel] = {
                "path": rel,
                "size": size,
                "mtime": mtime,
                "content_hash": _sha1_file(path),
                "last_indexed_ts": dt.datetime.now().isoformat(timespec="seconds"),
                "chunk_ids": chunk_ids,
            }
            chunks_added += len(chunk_ids)

        self._save_manifest(manifest)
        self._save_chunks(chunk_map)
        self._save_bm25(bm25)
        self._save_vector_store(vectors)
        elapsed = (dt.datetime.now() - start_time).total_seconds()
        self._write_report(
            files_scanned=files_scanned,
            files_changed=len(changed_files),
            files_added=files_added,
            files_deleted=len(deleted_paths),
            chunks_total=len(chunk_map),
            new_chunks=chunks_added,
            embeddings=embeddings_computed,
            repo_index_loaded=repo_index_loaded,
            elapsed=elapsed,
        )
        self._write_cache(
            {
                "last_run": dt.datetime.now().isoformat(timespec="seconds"),
                "files_scanned": files_scanned,
                "chunks_total": len(chunk_map),
            }
        )
        self.log(f"Index files written: {', '.join([MANIFEST_FILE, CHUNKS_FILE, BM25_FILE, VECTOR_FILE, REPORT_FILE])}")
        self._flush_log()
        return {
            "files_scanned": files_scanned,
            "files_changed": len(changed_files),
            "files_added": files_added,
            "files_deleted": len(deleted_paths),
            "chunks_total": len(chunk_map),
            "chunks_added": chunks_added,
            "embeddings": embeddings_computed,
            "elapsed": elapsed,
            "repo_index_loaded": repo_index_loaded,
        }
