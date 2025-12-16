# =============================================================================
# DevTools patch (based on latest DevTools zip state)
# - sots_tools.py: add menu entries to run Context Anchor tools (12/13)
# - save_context_anchor.py: robust plugin-folder resolution (aliases + heuristics)
# Result: anchors land in Plugins/<RealPluginFolder>/Docs/Anchor/ e.g.
#   E:\SAS\ShadowsAndShurikens\Plugins\BEP\Docs\Anchor\
# =============================================================================

[SOTS_DEVTOOLS]
tool: write_files
category: devtools
plugin: DevTools
pass: CONTEXT_ANCHOR_ROUTING_V2
[/SOTS_DEVTOOLS]

=== FILE: DevTools/python/sots_tools.py ===
from __future__ import annotations

import sys
import subprocess
import argparse
import json
from pathlib import Path
import datetime  # NEW: for pretty timestamps in manual gateway

# ---------------------------------------------------------------------------
# Toolbox metadata
# ---------------------------------------------------------------------------

TOOLBOX_VERSION = "SOTS DevTools Toolbox v1.6"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def get_tools_root() -> Path:
    return Path(__file__).resolve().parent


def run_script(script_name: str, extra_args: list[str] | None = None) -> None:
    """
    Helper to run a Python tool script located in DevTools/python.

    - If the script is missing, print a clear error and return.
    - All tools are responsible for their own confirm_start/confirm_exit,
      logging via llm_log, etc.
    """
    tools_root = get_tools_root()
    script = tools_root / script_name

    if not script.exists():
        print(f"[ERROR] Script not found: {script}")
        return

    cmd: list[str] = [sys.executable, str(script)]
    if extra_args:
        cmd.extend(extra_args)

    print(f"[INFO] Running: {' '.join(cmd)}")
    subprocess.run(cmd, check=False)


def run_pack(pack_path: str) -> int:
    """
    Run a ChatGPT pack (write_files) manually via the same pathway as inbox files.

    The pack_path is passed to write_files.py --source so behavior/logging match
    existing manual runs. All stdout/stderr stream through unchanged.
    """
    script = get_tools_root() / "write_files.py"
    if not script.exists():
        print(f"[ERROR] write_files.py not found at {script}")
        return 1

    pack = Path(pack_path).resolve()
    cmd: list[str] = [sys.executable, str(script), "--source", str(pack)]
    print(f"[INFO] Running pack: {' '.join(cmd)}")
    return subprocess.call(cmd)


# ---------------------------------------------------------------------------
# Audits
# ---------------------------------------------------------------------------

def run_aip_audit(project: str | None = None, out: str | None = None) -> int:
    try:
        import aip_audit_configs  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import aip_audit_configs: {exc}")
        return 1

    args: list[str] = []
    if project:
        args += ["--project", project]
    if out:
        args += ["--out", out]
    return aip_audit_configs.main(args)


def run_tag_audit(project: str | None = None, out: str | None = None) -> int:
    try:
        import tag_audit  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import tag_audit: {exc}")
        return 1

    args: list[str] = []
    if project:
        args += ["--project", project]
    if out:
        args += ["--out", out]
    return tag_audit.main(args)


def run_uplugin_audit(project: str | None = None, out: str | None = None) -> int:
    try:
        import uplugin_audit  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import uplugin_audit: {exc}")
        return 1

    args: list[str] = []
    if project:
        args += ["--project", project]
    if out:
        args += ["--out", out]
    return uplugin_audit.main(args)


def run_bep_export_audit(project: str | None = None, out: str | None = None) -> int:
    try:
        import bep_export_audit  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import bep_export_audit: {exc}")
        return 1

    args: list[str] = []
    if project:
        args += ["--project", project]
    if out:
        args += ["--out", out]
    return bep_export_audit.main(args)


# ---------------------------------------------------------------------------
# Convenience runners (call scripts so behavior/logging stays consistent)
# ---------------------------------------------------------------------------

def run_clean_binaries_intermediate() -> None:
    run_script("clean_binaries_intermediate.py")


def run_analyze_build_log() -> None:
    run_script("analyze_build_log.py")


def run_summarize_crash_logs() -> None:
    run_script("summarize_crash_logs.py")


def run_scan_todos() -> None:
    run_script("scan_todos.py")


def run_architecture_lint() -> None:
    run_script("architecture_lint.py")


def run_fsots_duplicate_report() -> None:
    run_script("fsots_duplicate_report.py")


def run_project_health_report() -> None:
    run_script("project_health_report.py")


def run_plugin_stats() -> None:
    run_script("plugin_stats.py")


def run_include_map() -> None:
    run_script("include_map.py")


def run_mass_regex_edit() -> None:
    run_script("mass_regex_edit.py")


def run_inject_license_header() -> None:
    run_script("inject_license_header.py")


def run_apply_json_pack() -> None:
    run_script("apply_json_pack.py")


def run_ad_hoc_regex_search() -> None:
    run_script("ad_hoc_regex_search.py")


def run_quick_search() -> None:
    run_script("quick_search.py")


def run_directory_index() -> None:
    run_script("generate_directory_index.py")


def run_kem_execution_report() -> None:
    run_script("kem_telemetry_report.py")


def run_kem_coverage_matrix_report() -> None:
    run_script("kem_coverage_matrix_report.py")


# ---------------------------------------------------------------------------
# ChatGPT Inbox manual bridge (Category 4 helper)
# ---------------------------------------------------------------------------

def run_apply_latest_chatgpt_inbox() -> None:
    """
    Manually apply the newest file in DevTools/python/chatgpt_inbox by dispatching it
    through the same dispatcher used by the Send2SOTS bridge server.

    This keeps Copilot/Buddy aligned with your "manual gateway" rule: DevTools runs are
    manual by default, never assumed.
    """
    tools_root = get_tools_root()
    root = tools_root / "chatgpt_inbox"

    if not root.exists():
        print(f"[WARN] chatgpt_inbox directory does not exist yet: {root}")
        return

    try:
        # Import once so we fail early if the dispatcher is missing.
        from sots_chatgpt_dispatcher import dispatch_file  # type: ignore
    except Exception as exc:
        print("[ERROR] Could not import sots_chatgpt_dispatcher.dispatch_file.")
        print(f"       {exc}")
        print("       Make sure sots_chatgpt_dispatcher.py is on PYTHONPATH and defines:")
        print("           def dispatch_file(prompt_path: Path, *, force: bool = False) -> None")
        return

    current = root
    newest: Path | None = None

    # Find newest file anywhere under chatgpt_inbox
    for p in current.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in {".txt", ".md"}:
            continue
        try:
            if newest is None or p.stat().st_mtime > newest.stat().st_mtime:
                newest = p
        except OSError:
            continue

    if newest is None:
        print(f"[INFO] No files found under: {root}")
        return

    ts = datetime.datetime.fromtimestamp(newest.stat().st_mtime).strftime("%Y-%m-%d %H:%M:%S")
    print("")
    print("=== ChatGPT Inbox Manual Gateway ===")
    print(f"Selected newest file:\n  {newest}\n  (mtime: {ts})")
    print("")
    ans = input("Dispatch this file now? (y/N): ").strip().lower()
    if ans not in {"y", "yes"}:
        print("[INFO] Cancelled.")
        return

    dispatch_file(newest, force=False)
    print("[INFO] Dispatch complete.")


# ---------------------------------------------------------------------------
# Context Anchors
# ---------------------------------------------------------------------------

def run_save_context_anchor() -> None:
    """
    Save a [CONTEXT_ANCHOR] block and route it into Plugins/<Plugin>/Docs/Anchor/.
    Also writes a central copy under DevTools/python/Saved/ContextAnchors/.
    """
    run_script("save_context_anchor.py")


def run_scan_context_anchors_inbox() -> None:
    """
    Scan DevTools/python/chatgpt_inbox for files containing [CONTEXT_ANCHOR] blocks,
    then route them into Plugins/<Plugin>/Docs/Anchor/ and move processed files
    into DevTools/python/Saved/ContextAnchors/inbox_processed/.
    """
    run_script("save_context_anchor.py", ["--scan-inbox", "--move-processed"])


def category_core_maintenance() -> None:
    while True:
        print("")
        print("=== Category 1: Core maintenance ===")
        print("")
        print("  1) Clean Binaries/Intermediate (safe)")
        print("  2) Analyze last build log")
        print("  3) Summarize crash logs")
        print("  4) Scan TODO / FIXME comments")
        print("  5) Project health report")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Core> ").strip()

        if choice == "1":
            run_clean_binaries_intermediate()
        elif choice == "2":
            run_analyze_build_log()
        elif choice == "3":
            run_summarize_crash_logs()
        elif choice == "4":
            run_scan_todos()
        elif choice == "5":
            run_project_health_report()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_fsots_architecture() -> None:
    while True:
        print("")
        print("=== Category 2: Architecture / naming ===")
        print("")
        print("  1) Architecture lint (headers, forbidden patterns, etc.)")
        print("  2) Duplicate FSOTS struct report")
        print("  3) Include map (public/private includes) report")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Arch> ").strip()

        if choice == "1":
            run_architecture_lint()
        elif choice == "2":
            run_fsots_duplicate_report()
        elif choice == "3":
            run_include_map()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_plugins_dependencies() -> None:
    while True:
        print("")
        print("=== Category 3: Plugins / dependencies ===")
        print("")
        print("  1) Plugin stats report")
        print("  2) Plugin dependency health")
        print("  3) Ensure plugin modules exist")
        print("  4) Fix plugin dependencies")
        print("  5) Compare plugin zips")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Plugins> ").strip()

        if choice == "1":
            run_plugin_stats()
        elif choice == "2":
            run_script("plugin_dependency_health.py")
        elif choice == "3":
            run_script("ensure_plugin_modules.py")
        elif choice == "4":
            run_script("fix_plugin_dependencies.py")
        elif choice == "5":
            run_script("compare_plugin_zips.py")
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_batch_editing() -> None:
    while True:
        print("")
        print("=== Category 4: Batch editing / licensing / logs ===")
        print("")
        print("  1) Mass regex edit from config")
        print("  2) Inject / verify license headers")
        print("  3) Analyze last build log")
        print("  4) Summarize crash logs")
        print("  5) Scan TODO / FIXME comments")
        print("  6) Apply JSON DevTools pack")
        print("  7) Ad-hoc regex search (pattern + optional literal)")
        print("  8) [KEM] Execution & Position Report")
        print("  9) Quick search (literal + optional regex)")
        print(" 10) Generate directory index (LLM_DIRECTORY_INDEX.txt)")
        print(" 11) Browse ChatGPT inbox (manual dispatcher)")
        print(" 12) Save Context Anchor (paste/file)")
        print(" 13) Scan inbox for Context Anchors")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Batch> ").strip()

        if choice == "1":
            run_mass_regex_edit()
        elif choice == "2":
            run_inject_license_header()
        elif choice == "3":
            run_analyze_build_log()
        elif choice == "4":
            run_summarize_crash_logs()
        elif choice == "5":
            run_scan_todos()
        elif choice == "6":
            run_apply_json_pack()
        elif choice == "7":
            run_ad_hoc_regex_search()
        elif choice == "8":
            run_kem_execution_report()
        elif choice == "9":
            run_quick_search()
        elif choice == "10":
            run_directory_index()
        elif choice == "11":
            run_apply_latest_chatgpt_inbox()
        elif choice == "12":
            run_save_context_anchor()
        elif choice == "13":
            run_scan_context_anchors_inbox()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_high_level_checks() -> None:
    while True:
        print("")
        print("=== Category 5: High-level project checks ===")
        print("")
        print("  1) List available pipelines")
        print("  2) Run pipeline by name")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Checks> ").strip()

        if choice == "1":
            run_script("list_pipelines.py")
        elif choice == "2":
            name = input("Pipeline name> ").strip()
            if name:
                run_script("run_pipeline_by_name.py", [name])
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_kem_tools() -> None:
    while True:
        print("")
        print("=== Category 6: KillExecutionManager (KEM) tools ===")
        print("")
        print("  1) [KEM] Execution & Position Report")
        print("  2) [KEM] Coverage Matrix Report")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("KEM> ").strip()

        if choice == "1":
            run_kem_execution_report()
        elif choice == "2":
            run_kem_coverage_matrix_report()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def main() -> int:
    while True:
        print("")
        print("=== SOTS DevTools Hub ===")
        print(f"Version: {TOOLBOX_VERSION}")
        print("")
        print("  1) Core maintenance")
        print("  2) Architecture / naming")
        print("  3) Plugins / dependencies")
        print("  4) Batch editing / licensing / logs")
        print("  5) High-level project checks")
        print("  6) KillExecutionManager (KEM) tools")
        print("")
        print("  0) Exit")
        print("")

        choice = input("Main> ").strip()

        if choice == "1":
            category_core_maintenance()
        elif choice == "2":
            category_fsots_architecture()
        elif choice == "3":
            category_plugins_dependencies()
        elif choice == "4":
            category_batch_editing()
        elif choice == "5":
            category_high_level_checks()
        elif choice == "6":
            category_kem_tools()
        elif choice in {"0", "q", "Q", "exit"}:
            return 0
        else:
            print("[WARN] Unknown option, please try again.")


if __name__ == "__main__":
    raise SystemExit(main())
=== END FILE ===

=== FILE: DevTools/python/save_context_anchor.py ===
# DevTools/python/save_context_anchor.py
from __future__ import annotations

import argparse
import datetime
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent  # E:\SAS\ShadowsAndShurikens
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "context_anchor.log"

ANCHOR_START = "[CONTEXT_ANCHOR]"
ANCHOR_END = "[/CONTEXT_ANCHOR]"

# ---------------------------------------------------------------------------
# Plugin folder resolution
# ---------------------------------------------------------------------------

# Short/alias -> actual plugin folder name under PROJECT_ROOT/Plugins/
# Keep this list small; resolution also includes acronym/fuzzy matching.
PLUGIN_ALIASES: dict[str, str] = {
    # Core SOTS abbreviations
    "SOTS_KEM": "SOTS_KillExecutionManager",
    "KEM": "SOTS_KillExecutionManager",
    "SOTS_GSM": "SOTS_GlobalStealthManager",
    "GSM": "SOTS_GlobalStealthManager",
    "SOTS_AIP": "SOTS_AIPerception",
    "AIP": "SOTS_AIPerception",
    "AIPERCEPTION": "SOTS_AIPerception",
    "SOTS_MD": "SOTS_MissionDirector",
    "MD": "SOTS_MissionDirector",
    "MISSIONDIRECTOR": "SOTS_MissionDirector",
    "MMSS": "SOTS_MMSS",
    "UDSBRIDGE": "SOTS_UDSBridge",
    # Common short names
    "INV": "SOTS_INV",
    "UI": "SOTS_UI",
    "STATS": "SOTS_Stats",
    "STEAM": "SOTS_Steam",
    "PROFILESHARED": "SOTS_ProfileShared",
    "TAGMANAGER": "SOTS_TagManager",
    "FX": "SOTS_FX_Plugin",
    "PARKOUR": "SOTS_Parkour",
    "EDUTIL": "SOTS_EdUtil",
    "DEBUG": "SOTS_Debug",
    "BLUEPRINTGEN": "SOTS_BlueprintGen",
    # Third-party / non-SOTS
    "BEP": "BEP",
    "OMNITRACE": "OmniTrace",
    "LIGHTPROBE": "LightProbePlugin",
    "BLUEPRINTCOMMENTLINKS": "BlueprintCommentLinks",
}


def _normalize_key(s: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", s.strip().lower())


def _acronym(name: str) -> str:
    # e.g. SOTS_KillExecutionManager -> SKEM
    parts = [p for p in re.split(r"[^A-Za-z0-9]+", name) if p]
    firsts = "".join(p[0] for p in parts if p)
    uppers = "".join(ch for ch in name if ch.isupper())
    return (firsts + uppers).upper()


def _list_plugin_folders() -> list[str]:
    plugins_root = PROJECT_ROOT / "Plugins"
    if not plugins_root.exists():
        return []
    out: list[str] = []
    for p in plugins_root.iterdir():
        if not p.is_dir():
            continue
        # Consider it a plugin folder if it contains a .uplugin file.
        if any(p.glob("*.uplugin")):
            out.append(p.name)
    return sorted(out)


def resolve_plugin_folder_name(plugin_id: str) -> tuple[str | None, str]:
    """
    Resolve a plugin identifier from an anchor (e.g. 'SOTS_KEM', 'Lock_SOTS_UI', 'BEP')
    into the actual plugin folder name under PROJECT_ROOT/Plugins.

    Returns: (folder_name_or_None, reason_string)
    """
    raw = plugin_id.strip()
    if not raw:
        return None, "empty"

    # Strip 'Lock_' prefix if present.
    if raw.lower().startswith("lock_"):
        raw = raw[5:]

    candidates = [raw]
    if raw.upper().startswith("SOTS_"):
        candidates.append(raw[5:])  # without SOTS_
    # Also try the last token after underscores.
    if "_" in raw:
        candidates.append(raw.split("_")[-1])

    plugin_folders = _list_plugin_folders()
    if not plugin_folders:
        return None, "no Plugins/* folders found"

    # Exact match (case-sensitive) and case-insensitive match.
    for c in candidates:
        direct = PROJECT_ROOT / "Plugins" / c
        if direct.exists():
            return c, "direct"
        for pf in plugin_folders:
            if pf.lower() == c.lower():
                return pf, "case-insensitive"

    # Alias table.
    for c in candidates:
        mapped = PLUGIN_ALIASES.get(c.strip().upper())
        if mapped:
            # mapped might still need case-insensitive resolution
            direct = PROJECT_ROOT / "Plugins" / mapped
            if direct.exists():
                return mapped, f"alias({c}->{mapped})"
            for pf in plugin_folders:
                if pf.lower() == mapped.lower():
                    return pf, f"alias-ci({c}->{pf})"

    # Acronym / heuristic matching.
    # Build acronym set for each folder.
    folder_meta = []
    for pf in plugin_folders:
        folder_meta.append((pf, _normalize_key(pf), _acronym(pf)))

    for c in candidates:
        c_norm = _normalize_key(c)
        c_up = c.strip().upper()
        hits: list[str] = []
        for pf, pf_norm, pf_acr in folder_meta:
            # substring on normalized form
            if c_norm and (c_norm in pf_norm or pf_norm in c_norm):
                hits.append(pf)
                continue
            # acronym contains candidate
            if c_up and c_up in pf_acr:
                hits.append(pf)
                continue
        hits = sorted(set(hits))
        if len(hits) == 1:
            return hits[0], f"heuristic({c}->{hits[0]})"
        if len(hits) > 1:
            return None, f"ambiguous({c} -> {hits})"

    return None, f"unresolved({plugin_id})"


def log(msg: str) -> None:
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line)
    try:
        LOG_FILE.write_text(LOG_FILE.read_text(encoding="utf-8", errors="replace") + line + "\n", encoding="utf-8")
    except Exception:
        # If the log file can't be read/written for some reason, still keep stdout.
        try:
            LOG_FILE.write_text(line + "\n", encoding="utf-8")
        except Exception:
            pass


def now_tag() -> str:
    return datetime.datetime.now().strftime("%Y%m%d_%H%M%S")


def safe_slug(s: str) -> str:
    s = s.strip().replace(" ", "_")
    s = re.sub(r"[^A-Za-z0-9_\-]+", "", s)
    return s[:64] if s else "ANCHOR"


def parse_anchor_blocks(text: str) -> list[str]:
    blocks: list[str] = []
    start = 0
    while True:
        i = text.find(ANCHOR_START, start)
        if i == -1:
            break
        j = text.find(ANCHOR_END, i + len(ANCHOR_START))
        if j == -1:
            # If end tag missing, take rest of text
            blocks.append(text[i:].strip())
            break
        blocks.append(text[i : j + len(ANCHOR_END)].strip())
        start = j + len(ANCHOR_END)
    return blocks


def infer_plugins(anchor_block: str) -> list[str]:
    """
    Try to infer one or more plugin folder names from within the anchor block.

    Supported patterns inside the block:
      - plugin: SOTS_UI
      - plugins: SOTS_UI, SOTS_INV
      - Lock_SOTS_UI (or any 'Lock_<PluginId>' token)
    """
    found: set[str] = set()

    # plugin: / plugins:
    for line in anchor_block.splitlines():
        m = re.match(r"^\s*plugins?\s*:\s*(.+)\s*$", line.strip(), flags=re.IGNORECASE)
        if m:
            raw = m.group(1)
            parts = re.split(r"[,\s]+", raw.strip())
            for p in parts:
                p = p.strip()
                if p:
                    found.add(p)

    # Lock_* tokens
    for m in re.finditer(r"\bLock_[A-Za-z0-9_\-]+\b", anchor_block):
        tok = m.group(0).strip()
        if tok:
            found.add(tok.replace("Lock_", "", 1))

    return sorted(found)


def infer_anchor_timestamp(anchor_block: str) -> str:
    """
    Try to infer a timestamp from a line like:
      date: 2025-12-16
      dt: 2025-12-16 07:30
      created: 20251216_073000
    Otherwise returns now_tag().
    """
    # ISO-ish date/time patterns
    m = re.search(r"\b(\d{4})[-/](\d{2})[-/](\d{2})(?:[ T](\d{2}):(\d{2})(?::(\d{2}))?)?\b", anchor_block)
    if m:
        y, mo, d = m.group(1), m.group(2), m.group(3)
        hh = m.group(4) or "00"
        mm = m.group(5) or "00"
        ss = m.group(6) or "00"
        try:
            dt = datetime.datetime(int(y), int(mo), int(d), int(hh), int(mm), int(ss))
            return dt.strftime("%Y%m%d_%H%M%S")
        except ValueError:
            pass

    # Compact timestamp pattern
    m2 = re.search(r"\b(\d{8})[_-](\d{6})\b", anchor_block)
    if m2:
        return f"{m2.group(1)}_{m2.group(2)}"

    # Date-only
    m3 = re.search(r"\b(\d{4})[-/](\d{2})[-/](\d{2})\b", anchor_block)
    if m3:
        y, mo, d = m3.group(1), m3.group(2), m3.group(3)
        try:
            dt = datetime.datetime(int(y), int(mo), int(d), 0, 0, 0)
            return dt.strftime("%Y%m%d_000000")
        except ValueError:
            pass

    return now_tag()


def infer_anchor_slug(anchor_block: str, default_slug: str | None = None) -> str:
    """
    Try to infer a slug from:
      title: Something
      name: Something
    Else uses default_slug or "ANCHOR".
    """
    for key in ("title", "name", "slug"):
        m = re.search(rf"^\s*{key}\s*:\s*(.+)\s*$", anchor_block, flags=re.IGNORECASE | re.MULTILINE)
        if m:
            return safe_slug(m.group(1))
    return safe_slug(default_slug or "ANCHOR")


def build_filename(plugin: str, anchor_block: str, default_slug: str | None = None) -> str:
    ts = infer_anchor_timestamp(anchor_block)
    slug = infer_anchor_slug(anchor_block, default_slug=default_slug)
    return f"{ts}__{safe_slug(plugin)}__{slug}.md"


def write_anchor_to_plugin(plugin: str, filename: str, anchor_block: str) -> Path | None:
    folder, reason = resolve_plugin_folder_name(plugin)
    if not folder:
        log(f"[WARN] Could not resolve plugin '{plugin}': {reason}")
        return None

    plugin_dir = PROJECT_ROOT / "Plugins" / folder
    if not plugin_dir.exists():
        # Shouldn't happen if resolution is correct, but keep it safe.
        log(f"[WARN] Plugin folder not found after resolve ({plugin} -> {folder}, {reason}): {plugin_dir}")
        return None

    if folder != plugin:
        log(f"Resolved plugin id '{plugin}' -> '{folder}' ({reason})")

    dest_dir = plugin_dir / "Docs" / "Anchor"
    dest_dir.mkdir(parents=True, exist_ok=True)

    dest = dest_dir / filename
    if dest.exists():
        # Avoid overwrite: add numeric suffix
        stem = dest.stem
        ext = dest.suffix or ".md"
        for i in range(1, 1000):
            cand = dest_dir / f"{stem}_{i:02d}{ext}"
            if not cand.exists():
                dest = cand
                break

    dest.write_text(anchor_block.strip() + "\n", encoding="utf-8")
    return dest


def also_write_central_copy(filename: str, anchor_block: str) -> Path:
    central = ROOT / "Saved" / "ContextAnchors"
    central.mkdir(parents=True, exist_ok=True)
    dest = central / filename
    if dest.exists():
        stem = dest.stem
        ext = dest.suffix or ".md"
        for i in range(1, 1000):
            cand = central / f"{stem}_{i:02d}{ext}"
            if not cand.exists():
                dest = cand
                break
    dest.write_text(anchor_block.strip() + "\n", encoding="utf-8")
    return dest


def process_text(text: str, *, default_slug: str | None = None) -> int:
    blocks = parse_anchor_blocks(text)
    if not blocks:
        log("No [CONTEXT_ANCHOR] blocks found in input.")
        return 2

    rc = 0
    for block in blocks:
        plugins = infer_plugins(block)
        if not plugins:
            # still save a central copy so it isn't lost
            fn = f"{infer_anchor_timestamp(block)}__UNSCOPED__{infer_anchor_slug(block, default_slug=default_slug)}.md"
            cpath = also_write_central_copy(fn, block)
            log(f"[OK] Saved UNSCOPED anchor to central: {cpath}")
            continue

        # If multiple plugin ids, write one file per plugin id
        for plugin in plugins:
            filename = build_filename(plugin, block, default_slug=default_slug)
            ppath = write_anchor_to_plugin(plugin, filename, block)
            cpath = also_write_central_copy(filename, block)

            if ppath:
                log(f"[OK] Saved anchor -> {ppath}")
            else:
                rc = 1
                log(f"[WARN] Plugin write failed for '{plugin}'. Central copy saved: {cpath}")

    return rc


def read_text_from_file(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def scan_inbox(*, move_processed: bool = False, default_slug: str | None = None) -> int:
    inbox = ROOT / "chatgpt_inbox"
    if not inbox.exists():
        log(f"[ERROR] chatgpt_inbox not found: {inbox}")
        return 2

    candidates: list[Path] = []
    for p in inbox.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in {".txt", ".md", ".log"}:
            continue
        try:
            txt = p.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue
        if ANCHOR_START in txt:
            candidates.append(p)

    if not candidates:
        log("No inbox files contain [CONTEXT_ANCHOR].")
        return 0

    log(f"Found {len(candidates)} inbox file(s) with anchors.")
    processed_dir = ROOT / "Saved" / "ContextAnchors" / "inbox_processed"
    processed_dir.mkdir(parents=True, exist_ok=True)

    rc = 0
    for p in sorted(candidates, key=lambda x: x.stat().st_mtime, reverse=True):
        log(f"Processing inbox file: {p}")
        try:
            txt = p.read_text(encoding="utf-8", errors="replace")
        except Exception as exc:
            rc = 1
            log(f"[WARN] Failed to read: {p} ({exc})")
            continue

        prc = process_text(txt, default_slug=default_slug)
        rc = max(rc, prc)

        if move_processed:
            dest = processed_dir / p.name
            if dest.exists():
                dest = processed_dir / f"{p.stem}_{now_tag()}{p.suffix}"
            try:
                p.replace(dest)
                log(f"Moved processed inbox file -> {dest}")
            except Exception as exc:
                rc = 1
                log(f"[WARN] Could not move processed file: {p} ({exc})")

    return rc


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Save [CONTEXT_ANCHOR] blocks into Plugins/<Plugin>/Docs/Anchor/")
    ap.add_argument("--file", type=str, default=None, help="Path to a text file containing one or more [CONTEXT_ANCHOR] blocks.")
    ap.add_argument("--slug", type=str, default=None, help="Default slug if anchor block does not include title/name/slug.")
    ap.add_argument("--scan-inbox", action="store_true", help="Scan DevTools/python/chatgpt_inbox for anchors and save them.")
    ap.add_argument("--move-processed", action="store_true", help="When scanning inbox, move processed files into Saved/ContextAnchors/inbox_processed/")
    args = ap.parse_args(argv)

    log("=== save_context_anchor.py start ===")

    if args.scan_inbox:
        rc = scan_inbox(move_processed=args.move_processed, default_slug=args.slug)
        log(f"=== save_context_anchor.py end (rc={rc}) ===")
        return rc

    if args.file:
        path = Path(args.file).expanduser()
        if not path.exists():
            log(f"[ERROR] File not found: {path}")
            return 2
        txt = read_text_from_file(path)
        rc = process_text(txt, default_slug=args.slug)
        log(f"=== save_context_anchor.py end (rc={rc}) ===")
        return rc

    # Interactive paste mode
    log("Paste your [CONTEXT_ANCHOR] block(s). End input with a single line containing only: EOF")
    buf: list[str] = []
    while True:
        line = sys.stdin.readline()
        if not line:
            break
        if line.strip() == "EOF":
            break
        buf.append(line)

    txt = "".join(buf)
    rc = process_text(txt, default_slug=args.slug)
    log(f"=== save_context_anchor.py end (rc={rc}) ===")
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
=== END FILE ===
