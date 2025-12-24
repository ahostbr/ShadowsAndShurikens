from __future__ import annotations

import hashlib
import importlib
from typing import Callable, List, Optional, Sequence


class EmbeddingModel:
    def __init__(self, name: str, dim: int, embed_fn: Callable[[Sequence[str]], List[float]]) -> None:
        self.name = name
        self.dim = dim
        self._embed_fn = embed_fn

    def embed(self, texts: Sequence[str]) -> List[float]:
        return self._embed_fn(texts)


class HashEmbedding:
    DIM = 64

    def __init__(self) -> None:
        self.name = "hash"
        self.dim = self.DIM

    def embed(self, texts: Sequence[str]) -> List[float]:
        text = texts[0] if texts else ""
        digest = hashlib.sha512(text.encode("utf-8")).digest()
        return [((byte / 127.5) - 1.0) for byte in digest[: self.DIM]]


class EmbeddingProvider:
    def __init__(
        self,
        backend: str = "sentence_transformers",
        model_name: Optional[str] = None,
        logger: Optional[Callable[[str], None]] = None,
    ) -> None:
        self.logger = logger or (lambda msg: print(f"[embeddings] {msg}"))
        self.backend = backend
        self.model_name = model_name
        self._model: Optional[EmbeddingModel] = None
        self._load_model()

    def _log(self, message: str) -> None:
        if self.logger:
            self.logger(message)

    def _load_model(self) -> None:
        if self.backend == "sentence_transformers":
            try:
                module = importlib.import_module("sentence_transformers")
                model_cls = getattr(module, "SentenceTransformer")
                model_name = self.model_name or "all-MiniLM-L6-v2"
                model = model_cls(model_name)
                dim = model.get_sentence_embedding_dimension()
                self._model = EmbeddingModel(
                    name=f"sentence_transformers::{model_name}",
                    dim=dim,
                    embed_fn=lambda texts: list(model.encode(list(texts), show_progress_bar=False)),
                )
                self._log(f"Loaded sentence-transformers model '{model_name}' (dim={dim}).")
                return
            except ImportError:
                self._log("sentence_transformers not installed; falling back to hash embeddings.")
                self.backend = "hash"
            except Exception as exc:  # pragma: no cover - best effort
                self._log(f"Failed to load sentence-transformers model: {exc!r}; falling back.")
                self.backend = "hash"

        if self.backend == "hash":
            self._model = EmbeddingModel(
                name="hash",
                dim=HashEmbedding.DIM,
                embed_fn=lambda texts: HashEmbedding().embed(texts),
            )
            self._log("Using deterministic hash embeddings.")

    def embed(self, text: str) -> List[float]:
        if not self._model:
            raise RuntimeError("Embedding model is not initialized.")
        return self._model.embed([text])

    def get_dim(self) -> int:
        if not self._model:
            raise RuntimeError("Embedding model is not initialized.")
        return self._model.dim

    def get_model_name(self) -> str:
        if not self._model:
            return "unknown"
        return self._model.name
