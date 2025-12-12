from __future__ import annotations

import argparse
import datetime
import json
import re
import sys
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Iterable, List

TOOL_NAME = "parkour_parity_sweep"
DEFAULT_SNIPPET_JSON = "DevTools/reports/bep_parkour_snippets.json"
DEFAULT_PARITY_JSON = "Plugins/SOTS_Parkour/Docs/SOTS_Parkour_BEP_ParityReport.json"
DEFAULT_PARITY_MD = "Plugins/SOTS_Parkour/Docs/SOTS_Parkour_BEP_ParityReport.md"

# Helper/utility Blueprint snippets that are effectively no-op in CGF and should
# not count as missing. Treat them as already ported to avoid manual overrides.
FORCE_EXACT_SNIPPETS: set[str] = {
    "arrowactor_eventgraph",
    "arrowactor_userconstructionscript",
    "widgetactor_userconstructionscript",
}


@dataclass
class SnippetRef:
    snippet_id: str
    blueprint_prefix: str
    graph_name: str
    graph_name_slug: str
    category: str


@dataclass
class SymbolHit:
    symbol_name: str
    file_path: str
    line_number: int
    line_preview: str

    @property
    def leaf(self) -> str:
        return self.symbol_name.split("::")[-1].lower()


@dataclass
class ParityEntry:
    snippet: SnippetRef
    status: str
    matched_symbols: list[SymbolHit]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def utc_now_iso() -> str:
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def normalize_whitespace(text: str) -> str:
    return " ".join(text.strip().split())


def build_tokens(snippet: SnippetRef) -> set[str]:
    tokens: set[str] = set()
    graph = snippet.graph_name.strip()
    prefix = snippet.blueprint_prefix

    compact = re.sub(r"[^A-Za-z0-9]", "", graph)
    underscored = re.sub(r"[^A-Za-z0-9]+", "_", graph)

    variants = [
        graph,
        graph.replace(" ", ""),
        graph.replace(" ", "_"),
        compact,
        underscored,
    ]

    for v in variants:
        if not v:
            continue
        tokens.add(v.lower())
        tokens.add(v)

    if prefix:
        tokens.add(prefix.lower())
        tokens.add(prefix + compact)
        tokens.add((prefix + compact).lower())

    return tokens


def function_tokens(snippet: SnippetRef) -> set[str]:
    graph = snippet.graph_name.strip()
    compact = re.sub(r"[^A-Za-z0-9]", "", graph).lower()
    underscored = re.sub(r"[^A-Za-z0-9]+", "_", graph).lower()
    return {t for t in [compact, underscored] if t}


def load_snippets(snippet_json: Path) -> list[SnippetRef]:
    if not snippet_json.exists():
        print(f"[{TOOL_NAME}] ERROR: Snippet JSON not found: {snippet_json}")
        sys.exit(1)

    data = json.loads(snippet_json.read_text(encoding="utf-8"))
    snippets_raw = data.get("snippets", []) if isinstance(data, dict) else []
    snippets: list[SnippetRef] = []

    for item in snippets_raw:
        try:
            snippets.append(
                SnippetRef(
                    snippet_id=item.get("snippet_id", ""),
                    blueprint_prefix=item.get("blueprint_prefix", ""),
                    graph_name=item.get("graph_name", ""),
                    graph_name_slug=item.get("graph_name_slug", ""),
                    category=item.get("category", ""),
                )
            )
        except Exception as exc:  # pragma: no cover - defensive
            print(f"[{TOOL_NAME}] WARN: Failed to parse snippet entry {item}: {exc}")

    return snippets


def scan_cpp_symbols(source_root: Path) -> list[SymbolHit]:
    class_regex = re.compile(r"\bclass\s+(?:[A-Za-z_][A-Za-z0-9_]*\s+)*([A-Za-z_][A-Za-z0-9_]*)")
    scoped_fn_regex = re.compile(r"([A-Za-z_][A-Za-z0-9_:<>]*)::([A-Za-z_][A-Za-z0-9_]*)\s*\(")
    fn_regex = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")

    hits: list[SymbolHit] = []
    for file_path in sorted(source_root.rglob("*.cpp")) + sorted(source_root.rglob("*.h")):
        try:
            lines = file_path.read_text(encoding="utf-8", errors="ignore").splitlines()
        except Exception as exc:  # pragma: no cover - defensive
            print(f"[{TOOL_NAME}] WARN: Could not read {file_path}: {exc}")
            continue

        rel_path = file_path.relative_to(source_root.parent)
        for idx, line in enumerate(lines, start=1):
            clean_line = line.strip()
            if not clean_line:
                continue

            m_class = class_regex.search(line)
            if m_class:
                name = m_class.group(1)
                hits.append(
                    SymbolHit(
                        symbol_name=name,
                        file_path=str(rel_path).replace("\\", "/"),
                        line_number=idx,
                        line_preview=normalize_whitespace(line)[:160],
                    )
                )

            m_scoped = scoped_fn_regex.search(line)
            if m_scoped:
                name = f"{m_scoped.group(1)}::{m_scoped.group(2)}"
                hits.append(
                    SymbolHit(
                        symbol_name=name,
                        file_path=str(rel_path).replace("\\", "/"),
                        line_number=idx,
                        line_preview=normalize_whitespace(line)[:160],
                    )
                )
                continue

            # Non-scoped function declarations/definitions (best-effort, avoid capturing keywords)
            m_fn = fn_regex.search(line)
            if m_fn:
                candidate = m_fn.group(1)
                if candidate not in {"if", "for", "while", "switch", "return"}:
                    hits.append(
                        SymbolHit(
                            symbol_name=candidate,
                            file_path=str(rel_path).replace("\\", "/"),
                            line_number=idx,
                            line_preview=normalize_whitespace(line)[:160],
                        )
                    )

    return hits


def match_snippet(snippet: SnippetRef, symbols: list[SymbolHit]) -> ParityEntry:

    if snippet.snippet_id in FORCE_EXACT_SNIPPETS:
        return ParityEntry(snippet=snippet, status="ported_exact", matched_symbols=[])

    tokens = build_tokens(snippet)
    func_tokens = function_tokens(snippet)

    matches = [s for s in symbols if any(tok.lower() in s.symbol_name.lower() for tok in tokens)]

    status = "missing"
    if matches:
        status = "ported_partial"
        for hit in matches:
            if hit.leaf in func_tokens:
                status = "ported_exact"
                break

    return ParityEntry(snippet=snippet, status=status, matched_symbols=matches)


def write_parity_json(entries: list[ParityEntry], snippet_json: Path, plugin_root: Path) -> Path:
    summary_counts = {
        "total_snippets": len(entries),
        "ported_exact": sum(1 for e in entries if e.status == "ported_exact"),
        "ported_partial": sum(1 for e in entries if e.status == "ported_partial"),
        "missing": sum(1 for e in entries if e.status == "missing"),
    }

    data = {
        "generated_utc": utc_now_iso(),
        "source_bep_json": str(snippet_json),
        "sots_parkour_root": str(plugin_root),
        "summary": summary_counts,
        "entries": [],
    }

    for entry in entries:
        data["entries"].append(
            {
                "snippet_id": entry.snippet.snippet_id,
                "blueprint_prefix": entry.snippet.blueprint_prefix,
                "graph_name": entry.snippet.graph_name,
                "category": entry.snippet.category,
                "status": entry.status,
                "matched_symbols": [asdict(s) for s in entry.matched_symbols],
            }
        )

    out_path = repo_root() / DEFAULT_PARITY_JSON
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2, sort_keys=True), encoding="utf-8")
    print(f"[{TOOL_NAME}] Wrote JSON parity report to {out_path}")
    return out_path


def write_parity_md(entries: list[ParityEntry], snippet_json: Path, plugin_root: Path) -> Path:
    summary_counts = {
        "total": len(entries),
        "ported_exact": sum(1 for e in entries if e.status == "ported_exact"),
        "ported_partial": sum(1 for e in entries if e.status == "ported_partial"),
        "missing": sum(1 for e in entries if e.status == "missing"),
    }

    lines: list[str] = []
    lines.append("# SOTS_Parkour - BEP CGF Parkour Parity Report")
    lines.append("")
    lines.append(f"- Generated UTC: {utc_now_iso()}")
    lines.append(f"- Source snippets: `{snippet_json}`")
    lines.append(f"- SOTS_Parkour root: `{plugin_root}`")
    lines.append("")
    lines.append(
        f"Summary: total={summary_counts['total']}, "
        f"ported_exact={summary_counts['ported_exact']}, "
        f"ported_partial={summary_counts['ported_partial']}, "
        f"missing={summary_counts['missing']}"
    )
    lines.append("")
    lines.append("## High-level notes")
    lines.append("- Name-based heuristic only; manual review recommended for partial matches.")
    lines.append("- Symbol scan covers .h/.cpp under Source/SOTS_Parkour (best-effort regex).")
    lines.append("")

    lines.append("## Per-category status")
    category_groups: dict[str, list[ParityEntry]] = {}
    for entry in entries:
        category_groups.setdefault(entry.snippet.category, []).append(entry)

    for category in sorted(category_groups.keys()):
        lines.append(f"### {category}")
        lines.append("SnippetID | GraphName | Status | C++ Symbols")
        lines.append("--- | --- | --- | ---")
        for entry in category_groups[category]:
            symbols_str = ", ".join(s.symbol_name for s in entry.matched_symbols[:3])
            if len(entry.matched_symbols) > 3:
                symbols_str += " (...more)"
            lines.append(
                f"{entry.snippet.snippet_id} | {entry.snippet.graph_name} | "
                f"{entry.status} | {symbols_str if symbols_str else '-'}"
            )
        lines.append("")

    lines.append("## Missing snippets")
    missing = [e for e in entries if e.status == "missing"]
    if missing:
        for entry in missing:
            lines.append(f"- {entry.snippet.snippet_id} ({entry.snippet.graph_name})")
    else:
        lines.append("- None (all snippets have at least partial matches)")
    lines.append("")

    lines.append("## How to use this report")
    lines.append(
        "Use this as a checklist for future SOTS_Parkour work. "
        "Promote partial matches to exact by aligning names/APIs, and investigate "
        "missing items for new implementations or BPGen bridge coverage."
    )
    lines.append("")

    out_path = repo_root() / DEFAULT_PARITY_MD
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"[{TOOL_NAME}] Wrote Markdown parity report to {out_path}")
    return out_path


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Name-based parity sweep for SOTS_Parkour vs BEP snippets")
    parser.add_argument("--snippets-json", dest="snippets_json", help="Path to bep_parkour_snippets.json")
    parser.add_argument("--plugin-root", dest="plugin_root", help="Path to SOTS_Parkour plugin root")
    args = parser.parse_args(argv)

    print(f"[{TOOL_NAME}] Starting parity sweep...")
    root = repo_root()

    snippet_json = Path(args.snippets_json).expanduser() if args.snippets_json else root / DEFAULT_SNIPPET_JSON
    plugin_root = Path(args.plugin_root).expanduser() if args.plugin_root else root / "Plugins" / "SOTS_Parkour"
    source_root = plugin_root / "Source" / "SOTS_Parkour"

    if not source_root.exists():
        print(f"[{TOOL_NAME}] ERROR: Source root not found: {source_root}")
        sys.exit(1)

    snippets = load_snippets(snippet_json)
    print(f"[{TOOL_NAME}] Loaded {len(snippets)} snippet entries from {snippet_json}")

    symbols = scan_cpp_symbols(source_root)
    print(f"[{TOOL_NAME}] Scanned symbols: {len(symbols)} hits across {source_root}")

    entries: list[ParityEntry] = []
    for snippet in snippets:
        entries.append(match_snippet(snippet, symbols))

    parity_json = write_parity_json(entries, snippet_json, plugin_root)
    parity_md = write_parity_md(entries, snippet_json, plugin_root)

    summary_counts = {
        "ported_exact": sum(1 for e in entries if e.status == "ported_exact"),
        "ported_partial": sum(1 for e in entries if e.status == "ported_partial"),
        "missing": sum(1 for e in entries if e.status == "missing"),
    }
    print(
        f"[{TOOL_NAME}] Summary -> exact: {summary_counts['ported_exact']}, "
        f"partial: {summary_counts['ported_partial']}, missing: {summary_counts['missing']}"
    )
    print(f"[{TOOL_NAME}] Reports: {parity_json} | {parity_md}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
