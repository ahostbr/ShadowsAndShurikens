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
    subprocess.run(cmd, check=False)
    return 0


def list_pipelines() -> None:
    """
    List available pipeline definitions.

    Sources:
    - DevToolsPipelines.json
    - DevTools/python/pipelines/*.json
    """
    tools_root = get_tools_root()
    root_json = tools_root.parent / "DevToolsPipelines.json"
    pipe_dir = tools_root / "pipelines"

    print("")
    print("=== Available pipelines ===")
    print("")

    def _print_one(name: str, payload: dict) -> None:
        desc = payload.get("description", "").strip()
        print(f"- {name}")
        if desc:
            print(f"    {desc}")

    # Root DevToolsPipelines.json
    if root_json.exists():
        try:
            payload = json.loads(root_json.read_text(encoding="utf-8", errors="replace"))
            for name, item in payload.get("pipelines", {}).items():
                _print_one(name, item)
        except Exception as exc:
            print(f"[WARN] Failed to read {root_json}: {exc}")

    # pipelines/*.json
    if pipe_dir.exists():
        for p in sorted(pipe_dir.glob("*.json")):
            try:
                item = json.loads(p.read_text(encoding="utf-8", errors="replace"))
                name = item.get("name", p.stem)
                _print_one(name, item)
            except Exception as exc:
                print(f"[WARN] Failed to read {p}: {exc}")

    print("")


def run_pipeline_by_name(pipeline_name: str) -> int:
    """
    Run a named pipeline via sots_pipeline_hub.py.

    This keeps the hub as the single entry point and preserves its logging.
    """
    tools_root = get_tools_root()
    hub_script = tools_root / "sots_pipeline_hub.py"
    if not hub_script.exists():
        print(f"[ERROR] Missing pipeline hub script: {hub_script}")
        return 1

    cmd: list[str] = [sys.executable, str(hub_script), pipeline_name]
    print(f"[INFO] Running pipeline: {' '.join(cmd)}")
    subprocess.run(cmd, check=False)
    return 0


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
        args.extend(["--project", project])
    if out:
        args.extend(["--out", out])

    print(f"[INFO] Running AIP audit (aip_audit_configs.py) with args: {args or 'default'}")
    return int(aip_audit_configs.main(args))


def run_udsbridge_audit(project: str | None = None, out: str | None = None) -> int:
    try:
        import udsbridge_audit_configs  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import udsbridge_audit_configs: {exc}")
        return 1

    args: list[str] = []
    if project:
        args.extend(["--project", project])
    if out:
        args.extend(["--out", out])

    print(f"[INFO] Running UDSBridge audit with args: {args or 'default'}")
    return int(udsbridge_audit_configs.main(args))


# ---------------------------------------------------------------------------
# Core maintenance (Category 1)
# ---------------------------------------------------------------------------

def run_clean_binaries_intermediate() -> None:
    run_script("clean_binaries_intermediate.py")


def run_analyze_build_log() -> None:
    run_script("analyze_build_log.py")


def run_summarize_crash_logs() -> None:
    run_script("summarize_crash_logs.py")


def run_scan_todos() -> None:
    run_script("scan_todos.py")


# ---------------------------------------------------------------------------
# Architecture / naming (Category 2)
# ---------------------------------------------------------------------------

def run_fsots_duplicate_report() -> None:
    run_script("fsots_duplicate_report.py")


def run_architecture_lint() -> None:
    run_script("architecture_lint.py")


# ---------------------------------------------------------------------------
# Plugins / dependency health (Category 3)
# ---------------------------------------------------------------------------

def run_plugin_dependency_health() -> None:
    run_script("plugin_dependency_health.py")


def run_ensure_plugin_modules() -> None:
    run_script("ensure_plugin_modules.py")


def run_fix_plugin_dependencies() -> None:
    run_script("fix_plugin_dependencies.py")


def run_compare_plugin_zips() -> None:
    run_script("compare_plugin_zips.py")


# ---------------------------------------------------------------------------
# Batch editing / licensing / logs (Category 4)
# ---------------------------------------------------------------------------

def run_mass_regex_edit() -> None:
    run_script("mass_regex_edit.py")


def run_inject_license_header() -> None:
    run_script("inject_license_header.py")


def run_apply_json_pack() -> None:
    """
    Apply a JSON pack (older format) using apply_json_pack.py.
    """
    run_script("apply_json_pack.py")


def run_ad_hoc_regex_search() -> None:
    """Run the ad-hoc regex search tool."""
    run_script("ad_hoc_regex_search.py")


def run_quick_search() -> None:
    """Run the quick search tool (literal + regex)."""
    run_script("quick_search.py")


def run_directory_index(
    root: str | None = None,
    out_dir: str | None = None,
    max_depth: int | None = None,
    folders_only: bool = False,
    exclude: str | None = None,
    chunk_lines: int | None = None,
) -> None:
    """
    Generate an LLM-friendly directory index (tree) text file.

    This is a thin wrapper that calls DevTools/python/generate_directory_index.py.
    """
    extra_args: list[str] = []
    if root:
        extra_args.extend(["--root", root])
    if out_dir:
        extra_args.extend(["--out-dir", out_dir])
    if max_depth is not None:
        extra_args.extend(["--max-depth", str(max_depth)])
    if folders_only:
        extra_args.append("--folders-only")
    if exclude:
        extra_args.extend(["--exclude", exclude])
    if chunk_lines is not None:
        extra_args.extend(["--chunk-lines", str(chunk_lines)])

    run_script("generate_directory_index.py", extra_args)


# ---------------------------------------------------------------------------
# KEM Tools (Category 6)
# ---------------------------------------------------------------------------

def run_kem_execution_report() -> None:
    run_script("kem_execution_report.py")


def run_kem_telemetry_report() -> None:
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
        if p.is_file():
            if newest is None or p.stat().st_mtime > newest.stat().st_mtime:
                newest = p

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

    # Dispatch (the dispatcher handles file type inference, pack applying, etc)
    dispatch_file(newest, force=False)
    print("[INFO] Dispatch complete.")




# ---------------------------------------------------------------------------
# Context Anchors (ADD-ONLY)
# ---------------------------------------------------------------------------

def run_save_context_anchor() -> None:
    """Save a [CONTEXT_ANCHOR] block to Plugins/<Plugin>/Docs/Anchor/."""
    run_script("save_context_anchor.py")


def run_scan_context_anchors_inbox() -> None:
    """Scan chatgpt_inbox for [CONTEXT_ANCHOR] blocks and route them to Plugins/<Plugin>/Docs/Anchor/."""
    run_script("save_context_anchor.py", ["--scan-inbox", "--move-processed"])

# ---------------------------------------------------------------------------
# Interactive menu UI
# ---------------------------------------------------------------------------

def category_core_maintenance() -> None:
    while True:
        print("")
        print("=== Category 1: Core maintenance ===")
        print("")
        print("  1) Clean Binaries/Intermediate (safe)")
        print("  2) Analyze last build log")
        print("  3) Summarize crash logs")
        print("  4) Scan TODO / FIXME comments")
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
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_fsots_architecture() -> None:
    while True:
        print("")
        print("=== Category 2: Architecture / naming ===")
        print("")
        print("  1) FSOTS duplicate report")
        print("  2) Architecture lint")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Arch> ").strip()

        if choice == "1":
            run_fsots_duplicate_report()
        elif choice == "2":
            run_architecture_lint()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_plugins_dependencies() -> None:
    while True:
        print("")
        print("=== Category 3: Plugins / dependencies ===")
        print("")
        print("  1) Plugin dependency health")
        print("  2) Ensure plugin modules")
        print("  3) Fix plugin dependencies (guided)")
        print("  4) Compare plugin zips")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Plugins> ").strip()

        if choice == "1":
            run_plugin_dependency_health()
        elif choice == "2":
            run_ensure_plugin_modules()
        elif choice == "3":
            run_fix_plugin_dependencies()
        elif choice == "4":
            run_compare_plugin_zips()
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

            run_apply_latest_chatgpt_inbox()
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
            list_pipelines()
        elif choice == "2":
            name = input("Pipeline name> ").strip()
            if name:
                run_pipeline_by_name(name)
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
        print("  2) [KEM] Telemetry Report")
        print("  3) [KEM] Coverage Matrix Report")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("KEM> ").strip()

        if choice == "1":
            run_kem_execution_report()
        elif choice == "2":
            run_kem_telemetry_report()
        elif choice == "3":
            run_kem_coverage_matrix_report()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def run_interactive_menu() -> None:
    tools_root = get_tools_root()
    print(f"[INFO] SOTS DevTools root: {tools_root}")

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
        elif choice in {"0", "q", "Q", "quit", "exit"}:
            print("[INFO] Exiting SOTS DevTools hub.")
            break
        else:
            print("[WARN] Unknown option, please try again.")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="SOTS DevTools hub (interactive menu by default)",
        add_help=True,
    )
    subparsers = parser.add_subparsers(dest="command")

    pack_parser = subparsers.add_parser(
        "run_pack",
        help="Run a write_files pack (saved in chatgpt_inbox or anywhere)",
    )
    pack_parser.add_argument("pack_path", help="Path to the prompt .txt file")

    pipeline_parser = subparsers.add_parser(
        "run_pipeline",
        help="Run a named pipeline from DevToolsPipelines.json / pipelines/*.json",
    )
    pipeline_parser.add_argument("pipeline_name", help="Name/ID of the pipeline to run")

    subparsers.add_parser(
        "list_pipelines",
        help="List available pipelines from DevToolsPipelines.json and pipelines/*.json",
    )

    subparsers.add_parser(
        "browse_inbox",
        help="Launch the inbox TUI browser (manual dispatcher)",
    )

    dir_parser = subparsers.add_parser(
        "dir-index",
        help="Generate an LLM-friendly directory index (tree) text file",
    )
    dir_parser.add_argument("--root", dest="root", help="Root directory to index (defaults to project root)")
    dir_parser.add_argument("--out-dir", dest="out_dir", help="Output directory for index files (defaults to DevTools/reports/directory_index)")
    dir_parser.add_argument("--max-depth", dest="max_depth", type=int, help="Maximum recursion depth (default: 10)")
    dir_parser.add_argument("--folders-only", dest="folders_only", action="store_true", help="Only list folders, not files")
    dir_parser.add_argument("--exclude", dest="exclude", help="Comma-separated exclude tokens (dir names or relative path prefixes)")
    dir_parser.add_argument("--chunk-lines", dest="chunk_lines", type=int, help="If set, split output into part files of N lines")

    subparsers.add_parser(
        "bpgen_packs",
        help="List BPGen snippet packs (DevTools/bpgen_snippets/packs)",
    )

    bpgen_run_parser = subparsers.add_parser(
        "bpgen_run_pack",
        help="Run all snippets in a BPGen pack via SOTS_BPGenBuildCommandlet",
    )
    bpgen_run_parser.add_argument("pack_name", help="Pack folder name under DevTools/bpgen_snippets/packs")
    bpgen_run_parser.add_argument("--ue-cmd", dest="ue_cmd", default="", help="Optional UE commandlet args")

    subparsers.add_parser(
        "bpgen_coverage",
        help="Run BPGen snippet coverage report",
    )

    # existing subcommands preserved below (BEP reports, parity sweeps, audits, etc.)
    report_parser = subparsers.add_parser(
        "bep-parkour-snippets",
        help="Inventory BEP-exported parkour snippets and write a JSON report",
    )
    report_parser.add_argument("--zip-path", dest="zip_path", help="Path to the BEP export zip")
    report_parser.add_argument("--output-dir", dest="output_dir", help="Output directory for reports (defaults to DevTools/reports)")

    parity_parser = subparsers.add_parser(
        "parkour-parity-sweep",
        help="Run name-based parity sweep between BEP snippets and SOTS_Parkour C++",
    )
    parity_parser.add_argument("--snippets-json", dest="snippets_json", help="Path to bep_parkour_snippets.json")
    parity_parser.add_argument("--plugin-root", dest="plugin_root", help="Root path to SOTS_Parkour plugin")

    aip_parser = subparsers.add_parser(
        "aip_audit",
        help="Run the AIPerception config audit",
    )
    aip_parser.add_argument("--project", dest="project", help="Project root override (defaults to detected root)")
    aip_parser.add_argument("--out", dest="out", help="Output report path (defaults to DevTools/logs/aip_audit_<timestamp>.txt)")

    udsbridge_parser = subparsers.add_parser(
        "udsbridge_audit",
        help="Scans all USOTS_UDSBridgeConfig assets for likely misconfigurations (UDS discovery, weather mappings, DLWE surface).",
    )
    udsbridge_parser.add_argument("--project", dest="project", help="Project root override (defaults to detected root)")
    udsbridge_parser.add_argument("--out", dest="out", help="Output report path (defaults to DevTools/logs/udsbridge_audit_<timestamp>.txt)")

    return parser


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    parser = build_arg_parser()
    parsed = parser.parse_args(args)

    if parsed.command is None:
        run_interactive_menu()
        return 0

    if parsed.command == "run_pack":
        return run_pack(parsed.pack_path)
    if parsed.command == "run_pipeline":
        return run_pipeline_by_name(parsed.pipeline_name)
    if parsed.command == "list_pipelines":
        list_pipelines()
        return 0
    if parsed.command == "browse_inbox":
        run_apply_latest_chatgpt_inbox()
        return 0
    if parsed.command == "dir-index":
        run_directory_index(
            parsed.root,
            parsed.out_dir,
            parsed.max_depth,
            parsed.folders_only,
            parsed.exclude,
            parsed.chunk_lines,
        )
        return 0
    if parsed.command == "bpgen_packs":
        return run_bpgen_packs()
    if parsed.command == "bpgen_run_pack":
        return run_bpgen_run_pack(parsed.pack_name, parsed.ue_cmd)
    if parsed.command == "bpgen_coverage":
        return run_bpgen_coverage()

    if parsed.command == "bep-parkour-snippets":
        run_bep_parkour_snippets_report(parsed.zip_path, parsed.output_dir)
        return 0
    if parsed.command == "parkour-parity-sweep":
        run_parkour_parity_sweep(parsed.snippets_json, parsed.plugin_root)
        return 0
    if parsed.command == "aip_audit":
        return run_aip_audit(parsed.project, parsed.out)
    if parsed.command == "udsbridge_audit":
        return run_udsbridge_audit(parsed.project, parsed.out)

    # If we somehow get here, fall back to interactive menu to preserve behavior.
    run_interactive_menu()
    return 0


if __name__ == "__main__":
    sys.exit(main())
