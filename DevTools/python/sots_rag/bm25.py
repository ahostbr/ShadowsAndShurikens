from __future__ import annotations

import math
import re
from collections import Counter
from typing import Dict, Iterable, List, Tuple

TOKEN_PATTERN = re.compile(r"[A-Za-z0-9_]+")


def tokenize(text: str) -> List[str]:
    return [tok.lower() for tok in TOKEN_PATTERN.findall(text)]


class BM25Index:
    SCHEMA_VERSION = 1

    def __init__(self, k1: float = 1.5, b: float = 0.75) -> None:
        self.k1 = k1
        self.b = b
        self.chunk_terms: Dict[str, Counter[str]] = {}
        self.doc_lengths: Dict[str, int] = {}
        self.term_doc_freq: Counter[str] = Counter()

    def _update_doc_freqs(self, chunk_id: str, freq: Counter[str], delta: int = 1) -> None:
        for term in freq.keys():
            self.term_doc_freq[term] += delta
            if self.term_doc_freq[term] <= 0:
                del self.term_doc_freq[term]

    def add_document(self, chunk_id: str, tokens: Iterable[str]) -> None:
        freq = Counter(tokens)
        if chunk_id in self.chunk_terms:
            self.remove_document(chunk_id)
        self.chunk_terms[chunk_id] = freq
        self.doc_lengths[chunk_id] = sum(freq.values())
        self._update_doc_freqs(chunk_id, freq, 1)

    def remove_document(self, chunk_id: str) -> None:
        freq = self.chunk_terms.pop(chunk_id, None)
        if not freq:
            return
        self._update_doc_freqs(chunk_id, freq, -1)
        self.doc_lengths.pop(chunk_id, None)

    def get_total_docs(self) -> int:
        return len(self.chunk_terms)

    def get_avg_doc_len(self) -> float:
        total = sum(self.doc_lengths.values())
        count = self.get_total_docs()
        if count == 0:
            return 0.0
        return total / count

    def _idf(self, term: str) -> float:
        df = self.term_doc_freq.get(term, 0)
        N = self.get_total_docs()
        return math.log(1 + (N - df + 0.5) / (df + 0.5)) if N else 0.0

    def score(self, query_tokens: Iterable[str], chunk_id: str) -> float:
        freq = self.chunk_terms.get(chunk_id)
        if not freq:
            return 0.0
        doc_len = self.doc_lengths.get(chunk_id, 0)
        avg_len = self.get_avg_doc_len()
        score = 0.0
        for term in query_tokens:
            term_freq = freq.get(term, 0)
            if term_freq == 0:
                continue
            idf = self._idf(term)
            denom = term_freq + self.k1 * (1 - self.b + self.b * (doc_len / avg_len if avg_len else 0))
            score += idf * ((term_freq * (self.k1 + 1)) / denom) if denom else 0.0
        return score

    def score_all(self, query_tokens: Iterable[str]) -> Dict[str, float]:
        tokens = list(query_tokens)
        if not tokens:
            return {}
        return {chunk_id: self.score(tokens, chunk_id) for chunk_id in self.chunk_terms}

    def to_dict(self) -> Dict[str, object]:
        return {
            "schema_version": self.SCHEMA_VERSION,
            "k1": self.k1,
            "b": self.b,
            "chunk_terms": {chunk_id: dict(counter) for chunk_id, counter in self.chunk_terms.items()},
            "doc_lengths": dict(self.doc_lengths),
            "term_doc_freq": dict(self.term_doc_freq),
        }

    @classmethod
    def from_dict(cls, data: Dict[str, object]) -> "BM25Index":
        inst = cls(k1=float(data.get("k1", 1.5)), b=float(data.get("b", 0.75)))
        chunk_terms = data.get("chunk_terms", {})
        inst.chunk_terms = {chunk_id: Counter(freq) for chunk_id, freq in chunk_terms.items()}
        inst.doc_lengths = {chunk_id: int(length) for chunk_id, length in data.get("doc_lengths", {}).items()}
        inst.term_doc_freq = Counter(data.get("term_doc_freq", {}))
        return inst
