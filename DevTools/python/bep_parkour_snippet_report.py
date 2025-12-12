from __future__ import annotations

import argparse
import datetime
import json
import re
import sys
import zipfile
from collections import Counter
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Iterable, List, Tuple


TOOL_NAME = "bep_parkour_snippet_report"
SNIPPET_PREFIX = "BlueprintFlows/Snippets/"
SNIPPET_SUFFIX = "_Snippet.txt"
DEFAULT_REPORT_BASENAME_JSON = "bep_parkour_snippets.json"
DEFAULT_REPORT_BASENAME_MD = "bep_parkour_snippets.md"
DEFAULT_LOG_BASENAME = "bep_parkour_snippet_report.log"


@dataclass
class SnippetEntry:
    snippet_id: str
    blueprint_prefix: str
    graph_name: str
    graph_name_slug: str
    category: str
    source_path_in_zip: str
    size_bytes: int


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def normalize_slug(text: str) -> str:
    lowered = text.lower()
    lowered = re.sub(r"[^a-z0-9]+", "-", lowered)
    return lowered.strip("-")


def utc_now_iso() -> str:
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def classify_prefix(prefix: str) -> str:
    if prefix.startswith("ABP"):
        return "ABP"
    if prefix == "ParkourComponent":
        return "CoreComponent"
    if prefix == "ParkourFunctionLibrary":
        return "FunctionLibrary"
    if prefix in {"ParkourInterface", "ParkourABPInterface"}:
        return "Interface"
    if prefix == "OutParkour":
        return "Debug/Output"
    if prefix == "ArrowActor":
        return "HelperActor"
    if prefix == "Location":
        return "HelperLocation"
    if prefix in {"WidgetActor", "ParkourStatsWidget"}:
        return "UI"
    if prefix.startswith("ReachLedgeIK") or prefix.startswith("CR_"):
        return "IK/ControlRig"
    if prefix.startswith("Editor_"):
        return "Editor"
    return "Misc"


def resolve_paths(zip_path_arg: str | None, output_dir_arg: str | None) -> tuple[Path, Path]:
    root = repo_root()
    candidates: list[Path] = []

    if zip_path_arg:
        candidates.append(Path(zip_path_arg).expanduser())
    else:
        candidates.extend(
            [
                root / "DevTools" / "BEP" / "BEP_EXPORT_CGF_ParkourComp.zip",
                root / "BEP_EXPORT_CGF_ParkourComp.zip",
                root / "BEP_EXPORT_CGF_ParkourComp",
            ]
        )

    resolved_zip: Path | None = None
    for candidate in candidates:
        if candidate.exists():
            resolved_zip = candidate
            break

    if resolved_zip is None:
        print(f"[{TOOL_NAME}] ERROR: Could not find BEP export. Tried: {[str(c) for c in candidates]}")
        sys.exit(1)

    if output_dir_arg:
        out_dir = Path(output_dir_arg).expanduser()
    else:
        out_dir = root / "DevTools" / "reports"

    out_dir.mkdir(parents=True, exist_ok=True)
    return resolved_zip, out_dir


def parse_snippet_name(stem: str) -> Tuple[str, str, str, str]:
    # Returns blueprint_prefix, graph_name, graph_slug, snippet_id
    trimmed = stem
    if trimmed.endswith("_Snippet"):
        trimmed = trimmed[: -len("_Snippet")]

    prefix = trimmed
    graph_name = ""
    if "_" in trimmed:
        prefix, graph_name = trimmed.split("_", 1)
    graph_slug = normalize_slug(graph_name)
    snippet_id = f"{prefix.lower()}_{graph_slug}" if graph_slug else prefix.lower()
    return prefix, graph_name, graph_slug, snippet_id


def iter_zip_snippets(zfile: zipfile.ZipFile) -> Iterable[Tuple[str, int]]:
    for info in zfile.infolist():
        name = info.filename
        if not name.startswith(SNIPPET_PREFIX):
            continue
        if not name.endswith(SNIPPET_SUFFIX):
            continue
        if info.is_dir():
            continue
        yield name, info.file_size


def iter_dir_snippets(base_dir: Path) -> Iterable[Tuple[str, int]]:
    for path in base_dir.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(base_dir).as_posix()
        if not rel.startswith(SNIPPET_PREFIX):
            continue
        if not rel.endswith(SNIPPET_SUFFIX):
            continue
        yield rel, path.stat().st_size


def build_entries(zip_path: Path) -> List[SnippetEntry]:
    entries: list[SnippetEntry] = []

    if zip_path.is_file():
        print(f"[{TOOL_NAME}] Reading zip: {zip_path}")
        with zipfile.ZipFile(zip_path, "r") as zf:
            iterator = iter_zip_snippets(zf)
            for source_path, size in iterator:
                stem = Path(source_path).stem
                prefix, graph_name, graph_slug, snippet_id = parse_snippet_name(stem)
                category = classify_prefix(prefix)
                entries.append(
                    SnippetEntry(
                        snippet_id=snippet_id,
                        blueprint_prefix=prefix,
                        graph_name=graph_name,
                        graph_name_slug=graph_slug,
                        category=category,
                        source_path_in_zip=source_path,
                        size_bytes=size,
                    )
                )
    elif zip_path.is_dir():
        print(f"[{TOOL_NAME}] Reading directory (fallback): {zip_path}")
        iterator = iter_dir_snippets(zip_path)
        for source_path, size in iterator:
            stem = Path(source_path).stem
            prefix, graph_name, graph_slug, snippet_id = parse_snippet_name(stem)
            category = classify_prefix(prefix)
            entries.append(
                SnippetEntry(
                    snippet_id=snippet_id,
                    blueprint_prefix=prefix,
                    graph_name=graph_name,
                    graph_name_slug=graph_slug,
                    category=category,
                    source_path_in_zip=source_path,
                    size_bytes=size,
                )
            )
    else:
        print(f"[{TOOL_NAME}] ERROR: Path is neither file nor directory: {zip_path}")
        sys.exit(1)

    entries.sort(key=lambda e: (e.category, e.blueprint_prefix, e.graph_name_slug, e.snippet_id))
    return entries


def write_json(entries: List[SnippetEntry], zip_path: Path, out_dir: Path) -> Path:
    data = {
        "source_zip": str(zip_path),
        "snippet_count": len(entries),
        "generated_utc": utc_now_iso(),
        "snippets": [asdict(e) for e in entries],
    }

    json_path = out_dir / DEFAULT_REPORT_BASENAME_JSON
    with json_path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, sort_keys=True)
    print(f"[{TOOL_NAME}] Wrote JSON to {json_path}")
    return json_path


def write_markdown(entries: List[SnippetEntry], zip_path: Path, out_dir: Path) -> Path:
    md_path = out_dir / DEFAULT_REPORT_BASENAME_MD
    generated = utc_now_iso()
    category_counts = Counter(e.category for e in entries)

    lines: list[str] = []
    lines.append("# BEP CGF Parkour Snippet Inventory")
    lines.append("")
    lines.append(f"- Source zip: `{zip_path}`")
    lines.append(f"- Generated UTC: {generated}")
    lines.append(f"- Snippet count: {len(entries)}")
    lines.append("")
    lines.append("## Category Summary")
    lines.append("")
    lines.append("Category | Count")
    lines.append("--- | ---")
    for category, count in sorted(category_counts.items(), key=lambda x: (-x[1], x[0])):
        lines.append(f"{category} | {count}")
    lines.append("")

    grouped: dict[str, list[SnippetEntry]] = {}
    for e in entries:
        grouped.setdefault(e.category, []).append(e)

    for category in sorted(grouped.keys()):
        lines.append(f"## {category}")
        lines.append("")
        lines.append("BlueprintPrefix | GraphName | ID | SourcePath")
        lines.append("--- | --- | --- | ---")
        for e in grouped[category]:
            lines.append(
                f"{e.blueprint_prefix} | {e.graph_name} | {e.snippet_id} | {e.source_path_in_zip}"
            )
        lines.append("")

    lines.append("## Next steps")
    lines.append("")
    lines.append(
        "This inventory is a reference for SOTS_Parkour parity and BPGen pack design. "
        "Use it to locate source snippets when planning ports or audits."
    )
    lines.append("")

    md_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"[{TOOL_NAME}] Wrote Markdown to {md_path}")
    return md_path


def write_log(out_dir: Path, messages: List[str]) -> Path:
    log_path = out_dir / DEFAULT_LOG_BASENAME
    log_path.write_text("\n".join(messages) + "\n", encoding="utf-8")
    return log_path


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="BEP Parkour snippet inventory")
    parser.add_argument("--zip-path", dest="zip_path", help="Path to BEP_EXPORT_CGF_ParkourComp.zip")
    parser.add_argument("--output-dir", dest="output_dir", help="Where to write reports (default: DevTools/reports)")
    args = parser.parse_args(argv)

    print(f"[{TOOL_NAME}] Starting report...")
    zip_path, out_dir = resolve_paths(args.zip_path, args.output_dir)
    print(f"[{TOOL_NAME}] Using zip path: {zip_path}")
    print(f"[{TOOL_NAME}] Output dir: {out_dir}")

    entries = build_entries(zip_path)
    if not entries:
        print(f"[{TOOL_NAME}] WARNING: No snippets found using prefix '{SNIPPET_PREFIX}'")

    json_path = write_json(entries, zip_path, out_dir)
    md_path = write_markdown(entries, zip_path, out_dir)

    category_counts = Counter(e.category for e in entries)
    summary_lines = [f"{cat}: {cnt}" for cat, cnt in sorted(category_counts.items(), key=lambda x: (-x[1], x[0]))]
    print(f"[{TOOL_NAME}] Total snippets: {len(entries)}")
    if summary_lines:
        print(f"[{TOOL_NAME}] Category counts -> {', '.join(summary_lines)}")

    log_messages = [
        f"source={zip_path}",
        f"output_dir={out_dir}",
        f"snippets={len(entries)}",
        f"json={json_path}",
        f"markdown={md_path}",
    ]
    log_path = write_log(out_dir, log_messages)
    print(f"[{TOOL_NAME}] Wrote log to {log_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
