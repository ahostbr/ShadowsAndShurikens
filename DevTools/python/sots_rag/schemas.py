from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List, Sequence

SCHEMA_VERSION = 1

MANIFEST_FILE = "rag_manifest.json"
CHUNKS_FILE = "rag_chunks.jsonl"
BM25_FILE = "rag_bm25_index.json"
VECTOR_FILE = "rag_vector_index.json"
REPORT_FILE = "rag_index_report.txt"
RUN_LOG_FILE = "rag_index_run.log"

QUERY_LOG_TEMPLATE = "rag_query_{timestamp}.log"
QUERY_REPORT_TEMPLATE = "rag_query_{timestamp}.txt"
QUERY_JSON_TEMPLATE = "rag_query_{timestamp}.json"

CHUNK_KINDS: Sequence[str] = ("code", "config", "doc")


@dataclass
class ChunkRecord:
    chunk_id: str
    path: str
    plugin: str = ""
    module: str = ""
    kind: str = ""
    start_line: int = 1
    end_line: int = 1
    text: str = ""
    symbol_hits: List[str] = field(default_factory=list)
    tag_hits: List[str] = field(default_factory=list)
    mtime: int = 0
    size: int = 0
    content_hash: str = ""

    def to_dict(self) -> Dict[str, object]:
        return {
            "chunk_id": self.chunk_id,
            "path": self.path,
            "plugin": self.plugin,
            "module": self.module,
            "kind": self.kind,
            "start_line": self.start_line,
            "end_line": self.end_line,
            "text": self.text,
            "symbol_hits": self.symbol_hits,
            "tag_hits": self.tag_hits,
            "mtime": self.mtime,
            "size": self.size,
            "content_hash": self.content_hash,
        }
