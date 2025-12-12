from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Dict, List

from cli_utils import confirm_start, confirm_exit
from llm_log import print_llm_summary
from project_paths import get_project_root

CALL_PATTERNS = {
    "RequestExecution_FromNinja": re.compile(r"\bRequestExecution_FromNinja\s*\("),
    "RequestExecution_FromDragon": re.compile(r"\bRequestExecution_FromDragon\s*\("),
    "RequestExecution_FromCinematic": re.compile(r"\bRequestExecution_FromCinematic\s*\("),
}

SOURCE_EXTENSIONS = {".h", ".hpp", ".cpp", ".c", ".cc", ".inl"}
CONTEXT_SEARCH_DEPTH = 12


@dataclass
class Callsite:
    path: Path
    line_no: int
    snippet: str
    context: str | None


# ---------------------------------------------------------------------------
# File scanning helpers
# ---------------------------------------------------------------------------

def iterate_source_files(root_dirs: Iterable[Path]) -> Iterable[Path]:
    for root in root_dirs:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix.lower() not in SOURCE_EXTENSIONS:
                continue
            yield path


def infer_context(lines: List[str], line_index: int) -> str | None:
    for offset in range(1, CONTEXT_SEARCH_DEPTH + 1):
        idx = line_index - offset
        if idx < 0:
            break
        candidate = lines[idx].strip()
        if not candidate or candidate.startswith("//"):
            continue
        if candidate.startswith(("UCLASS", "USTRUCT", "class ", "struct ")):
            return candidate
        if "(" in candidate and candidate.rstrip().endswith(")"):
            return candidate
    return None


def scan_file(path: Path) -> Dict[str, List[Callsite]]:
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return {}

    lines = text.splitlines()
    results: Dict[str, List[Callsite]] = {name: [] for name in CALL_PATTERNS}

    for idx, line in enumerate(lines, start=1):
        for call_name, pattern in CALL_PATTERNS.items():
            if pattern.search(line):
                context = infer_context(lines, idx - 1)
                results[call_name].append(
                    Callsite(
                        path=path,
                        line_no=idx,
                        snippet=line.strip(),
                        context=context,
                    )
                )
    return results


def format_path(path: Path, root: Path) -> str:
    try:
        return str(path.relative_to(root))
    except ValueError:
        return str(path)


def merge_results(aggregate: Dict[str, List[Callsite]], candidate: Dict[str, List[Callsite]]) -> None:
    for call_name, entries in candidate.items():
        aggregate.setdefault(call_name, []).extend(entries)


# ---------------------------------------------------------------------------
# Reporting helpers
# ---------------------------------------------------------------------------

def print_grouped_callsites(callsites: Dict[str, List[Callsite]], project_root: Path) -> None:
    print("[KEM Callsites]")
    for call_name, entries in callsites.items():
        print(f"{call_name}: {len(entries)} callsite{'' if len(entries) == 1 else 's'}")
        if not entries:
            print("  (none found)")
            continue
        for entry in entries:
            context = f" (context: {entry.context})" if entry.context else ""
            path_display = format_path(entry.path, project_root)
            print(f"  - {path_display}:{entry.line_no}{context}\n    {entry.snippet}")


def summarize_counts(callsites: Dict[str, List[Callsite]]) -> Dict[str, int]:
    return {name: len(entries) for name, entries in callsites.items()}


# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------

def main() -> None:
    confirm_start("kem_callsites_report")

    project_root = get_project_root()
    roots = [project_root / "Source", project_root / "Plugins"]

    aggregate: Dict[str, List[Callsite]] = {name: [] for name in CALL_PATTERNS}

    files_scanned = 0
    for path in iterate_source_files(roots):
        files_scanned += 1
        candidate = scan_file(path)
        merge_results(aggregate, candidate)

    total_callsites = sum(len(entries) for entries in aggregate.values())
    print(f"\nScanned {files_scanned} source files ({total_callsites} matching callsites).\n")
    print_grouped_callsites(aggregate, project_root)

    counts = summarize_counts(aggregate)
    print_llm_summary(
        "kem_callsites_report",
        files_scanned=files_scanned,
        callsite_counts=counts,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
