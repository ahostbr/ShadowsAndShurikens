# SOTS RAG DevTools

This package provides local retrieval-augmented tooling for the SAS repo. It builds
a chunked BM25/vector index plus bridges to the existing RepoIndex reports so that
future LLM sessions and local agents can answer questions without “re-researching.”

## Quick usage

### Index the repo (incremental by default)

```powershell
python DevTools/python/sots_rag/run_rag_index.py --project_root E:\SAS\ShadowsAndShurikens --verbose
```

### Query the index

```powershell
python DevTools/python/sots_rag/run_rag_query.py --project_root E:\SAS\ShadowsAndShurikens --q "Where is USOTS_KEMManagerSubsystem?" --top_k 12 --verbose
```

Docs-only query:
```powershell
python DevTools/python/sots_rag/run_rag_query.py --project_root E:\SAS\ShadowsAndShurikens --q "Core bridge passes" --scope docs --top_k 12
```

Code-only query:
```powershell
python DevTools/python/sots_rag/run_rag_query.py --project_root E:\SAS\ShadowsAndShurikens --q "RAGQuery _add_candidate" --scope code --top_k 12
```

## CLI options

### Indexer (`run_rag_index.py`)
- `--project_root`: project root path (auto-detected if omitted)
- `--reports_dir`: output dir (default `<root>/Reports/RAG`)
- `--include_code true|false`: include code/config files (default `true`)
- `--include_docs true|false`: include `.md`/`.txt` files (default `true`)
- `--full`: rebuild everything (ignores cache)
- `--changed_only true|false`: only reprocess touched files (default `true`)
- `--plugin_filter`: comma-separated glob list to restrict scanned plugins
- `--embedding_backend`: `sentence_transformers` (default) or `hash`
- `--embedding_model`: optional sentence-transformers model name
- `--verbose`: print progress to the console

### Query (`run_rag_query.py`)
- `--project_root`/`--reports_dir`: same defaults as the indexer
- `--q` (or positional): the question text
- `--top_k`: final hits to print (default 12)
- `--bm25_n`: BM25 candidates to score (default 20)
- `--vec_n`: vector candidates to score (default 20)
- `--rerank true|false` (default `false`): enable heuristic reranking
- `--rerank_k`: how many of the top hits to rerank (default 30)
- `--scope all|code|docs|config` (default `all`): restrict results by chunk kind
- `--embedding_backend`/`--embedding_model`: same as the indexer
- `--verbose`: prints the internal log envelope

## Outputs under `<PROJECT>/Reports/RAG/`

- `rag_manifest.json`: tracked file manifest + chunk references for incremental runs
- `rag_chunks.jsonl`: canonical chunk records with plugin/module metadata plus symbol/tag hits
- `rag_bm25_index.json`: BM25 stats (doc lengths, frequencies, schema version)
- `rag_vector_index.json`: stored vectors keyed by chunk_id
- `rag_index_report.txt`: human-readable summary of the latest index run
- `rag_index_run.log`: appended log lines from the indexer
- `rag_query_<timestamp>.txt/.json/.log`: query reports, machine output, and logs for each run

Additional cache/log messages live next to the tools:
- `_cache/rag_index_cache.json`
- `_logs/` (currently empty but reserved for future tooling/runtime logs)

## Index behavior

- **Chunking:** code (200–400 lines, 40-line overlap), configs (~250 lines), docs (~600 lines)
- **Metadata:** each chunk records path, plugin/module, kind, token ranges, symbol/tag hits, `chunk_id`, and file timestamps
- **Incremental:** `rag_manifest.json` tracks `mtime`, `size`, `content_hash`, and chunk_ids per file; deleted assets clean up existing entries
- **BM25:** pure Python BM25 implementation with per-chunk term frequencies and IDF rebuilt per run
- **Vectors:** uses SentenceTransformers (default); falls back to deterministic hash embeddings when the package is missing
- **RepoIndex bridge:** symbol/tag occurrences from `Reports/RepoIndex` enrich chunks and power “exact” lookups

## Query pipeline

1. RepoIndex exact matches (symbols/tags matched inside the query text)
2. BM25 lexical retrieval over `rag_chunks.jsonl`
3. Vector semantics over stored embeddings
4. Weighted merge (`exact > bm25 > vector`) with deduplication
5. Optional heuristic reranking (symbol/tag boosts + keyword density) if asked for

Each query prints a “Top hits” summary, writes structured JSON, a text report, and a log file. Hits include file path, line range, plugin/module, snippet (≤200 chars), and “How to open” guidance.

## Dependencies

- `sentence_transformers` is optional but recommended. Install via `pip install sentence_transformers`.
- The hash-backed embedding stays available when the model package is missing.

## Logging

- The indexer writes progress messages to both the console and `<Reports>/rag_index_run.log`.
- Queries emit `<Reports>/rag_query_<timestamp>.log` plus the JSON/text reports for later automation.
