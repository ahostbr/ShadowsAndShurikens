from __future__ import annotations

import math
from typing import Dict, Iterable, List, Mapping, Sequence, Tuple


class VectorStore:
    SCHEMA_VERSION = 1

    def __init__(self) -> None:
        self.vectors: Dict[str, List[float]] = {}
        self.norms: Dict[str, float] = {}

    def add(self, chunk_id: str, vector: Sequence[float]) -> None:
        vec = [float(v) for v in vector]
        norm = math.sqrt(sum(v * v for v in vec))
        self.vectors[chunk_id] = vec
        self.norms[chunk_id] = norm

    def remove(self, chunk_id: str) -> None:
        self.vectors.pop(chunk_id, None)
        self.norms.pop(chunk_id, None)

    def get_dim(self) -> int:
        if not self.vectors:
            return 0
        return len(next(iter(self.vectors.values())))

    def search(self, query_vector: Sequence[float], top_k: int = 10) -> List[Tuple[str, float]]:
        vec = [float(v) for v in query_vector]
        q_norm = math.sqrt(sum(v * v for v in vec))
        results: List[Tuple[str, float]] = []
        for chunk_id, stored in self.vectors.items():
            if len(stored) != len(vec):
                continue
            dot = sum(a * b for a, b in zip(vec, stored))
            denom = q_norm * self.norms.get(chunk_id, 0.0)
            score = dot / denom if denom > 0 else 0.0
            results.append((chunk_id, score))
        results.sort(key=lambda item: item[1], reverse=True)
        return results[:top_k]

    def to_dict(self) -> Dict[str, object]:
        return {
            "schema_version": self.SCHEMA_VERSION,
            "vectors": self.vectors,
        }

    @classmethod
    def from_dict(cls, data: Mapping[str, object]) -> "VectorStore":
        inst = cls()
        stored = data.get("vectors", {})
        for chunk_id, vector in stored.items():
            inst.add(chunk_id, vector)  # type: ignore[arg-type]
        return inst
