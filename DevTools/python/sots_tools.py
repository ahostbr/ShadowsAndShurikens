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
# Core helpers
# ---------------------------------------------------------------------------

def get_tools_root() -> Path:
    """
    Returns the DevTools/python directory (where this hub script lives).
    """
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
    print(f"[INFO] Running pack via write_files: {' '.join(cmd)}")
    result = subprocess.run(cmd, check=False, cwd=str(get_tools_root()))
    return result.returncode


def run_pipeline_by_name(pipeline_name: str) -> int:
    """
    Forward a named pipeline into the existing pipeline hub helpers.

    Preference order:
      1) Use run_bpgen_build.main if the hub exposes it (job-id style pipelines).
      2) If the hub exposes a callable run_pipeline(name), invoke it.
      3) Fallback: launch sots_pipeline_hub.py (interactive) to keep behavior
         unchanged when no direct entrypoint exists.
    """
    try:
        import sots_pipeline_hub as pipeline_hub  # type: ignore
    except Exception as exc:  # pragma: no cover - defensive
        print(f"[ERROR] Could not import sots_pipeline_hub: {exc}")
        return 1

    # Prefer the existing BPGen job runner pattern if available.
    run_bpgen_build = getattr(pipeline_hub, "run_bpgen_build", None)
    if run_bpgen_build is not None:
        main_fn = getattr(run_bpgen_build, "main", None)
        if callable(main_fn):
            print(f"[INFO] Running pipeline via run_bpgen_build: job-id={pipeline_name}")
            try:
                return int(main_fn(["--job-id", pipeline_name]))
            except Exception as exc:  # pragma: no cover - runtime guard
                print(f"[ERROR] run_bpgen_build raised: {exc}")

    # If the hub exposes a direct hook, use it.
    run_pipeline_fn = getattr(pipeline_hub, "run_pipeline", None)
    if callable(run_pipeline_fn):
        print(f"[INFO] Running pipeline via sots_pipeline_hub.run_pipeline('{pipeline_name}')")
        try:
            return int(run_pipeline_fn(pipeline_name))
        except Exception as exc:  # pragma: no cover - runtime guard
            print(f"[ERROR] run_pipeline threw: {exc}")

    # Fallback: launch the interactive hub so behavior stays consistent.
    hub_script = get_tools_root() / "sots_pipeline_hub.py"
    if hub_script.exists():
        cmd = [sys.executable, str(hub_script)]
        print(
            "[WARN] No direct pipeline handler exposed; launching pipeline hub interactively."
        )
        result = subprocess.run(cmd, check=False, cwd=str(get_tools_root()))
        return result.returncode

    print("[ERROR] sots_pipeline_hub.py not found; cannot run pipeline.")
    return 1


def list_pipelines() -> list[str]:
    """List available pipeline names from DevToolsPipelines.json and pipelines/*.json."""
    dev_root = get_tools_root().parent
    candidates: list[str] = []

    cfg_path = dev_root / "DevToolsPipelines.json"
    if cfg_path.exists():
        try:
            with cfg_path.open("r", encoding="utf-8") as f:
                data = json.load(f)
            pipelines = data.get("pipelines") if isinstance(data, dict) else None
            if isinstance(pipelines, list):
                for p in pipelines:
                    if isinstance(p, str):
                        candidates.append(p.strip())
        except Exception as exc:  # pragma: no cover - defensive
            print(f"[WARN] Failed to read DevToolsPipelines.json: {exc}")

    pipelines_dir = dev_root / "pipelines"
    if pipelines_dir.exists():
        for file in sorted(pipelines_dir.glob("*.json")):
            candidates.append(file.stem)

    # dedupe preserving order
    seen = set()
    result: list[str] = []
    for name in candidates:
        if not name:
            continue
        if name in seen:
            continue
        seen.add(name)
        result.append(name)

    for name in result:
        print(name)
    return result


# ---------------------------------------------------------------------------
# Tool launchers (no menu logic here, just wiring)
# ---------------------------------------------------------------------------

def run_open_unreal_project() -> None:
    """
    Launch the project's .uproject in Unreal Editor (Windows only).
    """
    run_script("run_unreal_project.py")


def run_clean_binaries_intermediate() -> None:
    run_script("clean_binaries_intermediate.py")


def run_scan_fsots_structs() -> None:
    run_script("scan_fsots_structs.py")


def run_architecture_lint() -> None:
    run_script("architecture_lint.py")


def run_mass_regex_edit() -> None:
    run_script("mass_regex_edit.py")


def run_inject_license_header() -> None:
    run_script("inject_license_header.py")


def run_analyze_build_log() -> None:
    run_script("analyze_build_log.py")


def run_summarize_crash_logs() -> None:
    run_script("summarize_crash_logs.py")


def run_package_plugin() -> None:
    run_script("package_plugin.py")


def run_plugin_audit() -> None:
    run_script("plugin_audit.py")


def run_compare_plugin_zips() -> None:
    run_script("compare_plugin_zips.py")


def run_scan_todos() -> None:
    run_script("scan_todos.py")


def run_project_health_report() -> None:
    run_script("project_health_report.py")


def run_ensure_plugin_modules() -> None:
    run_script("ensure_plugin_modules.py")


def run_build_and_analyze() -> None:
    run_script("run_build_and_analyze.py")


def run_fsots_duplicate_report() -> None:
    run_script("fsots_duplicate_report.py")


def run_plugin_dependency_health() -> None:
    run_script("plugin_dependency_health.py")


def run_kem_execution_report() -> None:
    try:
        from kem_execution_report import main as kem_execution_report_main
    except Exception as exc:
        print(f"[WARN] Failed to import kem_execution_report: {exc}. Falling back to subprocess.")
        run_script("kem_execution_report.py")
        return

    kem_execution_report_main()


def run_kem_callsites_report() -> None:
    run_script("kem_callsites_report.py")


def run_kem_decision_log_analyzer() -> None:
    run_script("kem_decision_log_analyzer.py")


def run_kem_coverage_report() -> None:
    run_script("kem_coverage_report.py")


def run_kem_anchor_report() -> None:
    run_script("kem_anchor_report.py")


def run_kem_omnitrace_tuning_report() -> None:
    run_script("kem_omnitrace_tuning_report.py")


def run_kem_telemetry_report() -> None:
    run_script("kem_telemetry_report.py")


def run_kem_coverage_matrix_report() -> None:
    run_script("kem_coverage_matrix_report.py")


def run_fix_plugin_dependencies() -> None:
    run_script("fix_plugin_dependencies.py")


def run_apply_json_pack() -> None:
    """
    Apply a JSON DevTools pack (file edits) using apply_json_pack.py.
    """
    run_script("apply_json_pack.py")


def run_ad_hoc_regex_search() -> None:
    """Run the ad-hoc regex search tool."""
    run_script("ad_hoc_regex_search.py")


def run_quick_search() -> None:
    """Run the quick search tool (literal + regex)."""
    run_script("quick_search.py")


def run_bep_parkour_snippets_report(zip_path: str | None = None, output_dir: str | None = None) -> None:
    extra_args: list[str] = []
    if zip_path:
        extra_args.extend(["--zip-path", zip_path])
    if output_dir:
        extra_args.extend(["--output-dir", output_dir])
    print("[INFO] Starting BEP parkour snippet inventory...")
    run_script("bep_parkour_snippet_report.py", extra_args)


def run_parkour_parity_sweep(snippets_json: str | None = None, plugin_root: str | None = None) -> None:
    extra_args: list[str] = []
    if snippets_json:
        extra_args.extend(["--snippets-json", snippets_json])
    if plugin_root:
        extra_args.extend(["--plugin-root", plugin_root])
    print("[INFO] Starting parkour parity sweep...")
    run_script("parkour_parity_sweep.py", extra_args)


def run_delete_target() -> None:
    """Run the delete_target tool to remove a file or folder by path."""
    run_script("delete_target.py")


# ---------------------------------------------------------------------------
# ChatGPT inbox manual gateway helpers
# ---------------------------------------------------------------------------

def get_chatgpt_inbox_dir() -> Path:
    """
    Returns the DevTools/python/chatgpt_inbox directory.
    """
    return get_tools_root() / "chatgpt_inbox"


def list_chatgpt_inbox(limit: int = 10) -> list[Path]:
    """
    List the latest ChatGPT inbox files (for debugging/manual runs).
    Returns the list of Path objects, newest first.
    """
    inbox = get_chatgpt_inbox_dir()
    if not inbox.exists():
        print(f"[WARN] chatgpt_inbox directory does not exist yet: {inbox}")
        return []

    files = sorted(
        inbox.glob("*.txt"),
        key=lambda p: p.stat().st_mtime,
        reverse=True,
    )
    if not files:
        print("[WARN] No .txt files found in chatgpt_inbox.")
        return []

    files_to_show = files[:limit] if limit > 0 else files

    print(f"[INFO] Latest {len(files_to_show)} ChatGPT inbox file(s):")
    for idx, p in enumerate(files_to_show, start=1):
        mtime = datetime.datetime.fromtimestamp(p.stat().st_mtime)
        print(f"  {idx:2d}) {p.name}  (modified {mtime})")

    return files

def run_apply_latest_chatgpt_inbox() -> None:
    """
    Interactive ChatGPT inbox browser:

    - Starts at DevTools/python/chatgpt_inbox.
    - Lets you navigate directories and select a .txt file.
    - When you select a file, it calls sots_chatgpt_dispatcher.dispatch_file(file, force=True).
    """
    root = get_chatgpt_inbox_dir()
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

    while True:
        print("")
        print("=== ChatGPT inbox browser ===")
        try:
            rel = "." if current == root else str(current.relative_to(root))
        except ValueError:
            # Safety: if something odd happens with paths, fall back to absolute.
            rel = str(current)
        print(f" Root : {root}")
        print(f" Here : {rel}")
        print("")

        # Collect dirs/files in this directory.
        dirs = sorted([p for p in current.iterdir() if p.is_dir()])
        files = sorted(
            [p for p in current.iterdir() if p.is_file() and p.suffix.lower() == ".txt"]
        )

        if not dirs and not files:
            print("  (No subdirectories or .txt files in this directory.)")

        index = 1
        if dirs:
            print("  Directories:")
            for d in dirs:
                print(f"   {index}) [D] {d.name}")
                index += 1
        if files:
            print("  Files:")
            for f in files:
                print(f"   {index}) [F] {f.name}")
                index += 1

        print("")
        print("  Commands:")
        if current != root:
            print("   b) Up one level")
        if files:
            print("   a) Execute ALL .txt files in this directory (sorted)")
        print("   r) Refresh")
        print("   q) Back to Batch menu")
        print("")

        choice = input("Inbox> ").strip()
        if not choice:
            continue

        lower = choice.lower()
        if lower in {"q", "0", "exit"}:
            print("[INFO] Leaving ChatGPT inbox browser.")
            return

        if lower == "r":
            continue

        if lower == "a":
            # Execute all .txt files in this directory, in the same sorted order
            # they are displayed above.
            if not files:
                print("[WARN] No .txt files to execute in this directory.")
                continue

            print("")
            print(f"[INFO] Execute ALL {len(files)} .txt file(s) in this directory (sorted by name):")
            for idx, f in enumerate(files, 1):
                try:
                    rel_file = f.relative_to(root)
                except ValueError:
                    rel_file = f
                print(f"   {idx:02d}) {rel_file}")

            confirm_all = input(
                "Run dispatcher on ALL of these files? [y/N]: "
            ).strip().lower()
            if confirm_all not in {"y", "yes"}:
                print("[INFO] Cancelled 'execute all'. Returning to browser.")
                continue

            for idx, f in enumerate(files, 1):
                try:
                    rel_file = f.relative_to(root)
                except ValueError:
                    rel_file = f
                print("")
                print(
                    f"[INFO] ({idx}/{len(files)}) "
                    f"Applying ChatGPT inbox message via dispatcher: {f}"
                )
                try:
                    dispatch_file(f, force=True)
                    print("[INFO]   -> Completed.")
                except Exception as exc2:
                    print(
                        f"[ERROR] Dispatcher raised an exception for {rel_file}: {exc2}"
                    )

            print("[INFO] Execute-all run completed. Returning to browser.")
            continue

        if lower == "b":
            if current != root:
                current = current.parent
            else:
                print("[WARN] Already at inbox root.")
            continue

        # Numeric selection.
        try:
            sel = int(choice)
        except ValueError:
            print("[WARN] Please enter a number, or one of b/a/r/q.")
            continue

        total = len(dirs) + len(files)
        if total == 0:
            print("[WARN] Nothing to select in this directory.")
            continue
        if sel < 1 or sel > total:
            print(f"[WARN] Selection out of range (1-{total}).")
            continue

        if sel <= len(dirs):
            # Go into a subdirectory.
            chosen_dir = dirs[sel - 1]
            current = chosen_dir
            continue

        # File selection.
        chosen_file = files[sel - len(dirs) - 1]
        try:
            rel_file = chosen_file.relative_to(root)
        except ValueError:
            rel_file = chosen_file

        print("")
        print(f"[INFO] Selected file: {rel_file}")
        confirm = input("Run dispatcher on this file? [y/N]: ").strip().lower()
        if confirm not in {"y", "yes"}:
            print("[INFO] Cancelled. Returning to browser.")
            continue

        print(f"[INFO] Applying ChatGPT inbox message via dispatcher: {chosen_file}")
        try:
            dispatch_file(chosen_file, force=True)
            print("[INFO] Manual dispatcher call completed (force=True).")
        except Exception as exc2:
            print(f"[ERROR] Dispatcher raised an exception: {exc2}")

        # After running once, return to the Batch menu. User can invoke again if needed.
        return


def run_bpgen_packs() -> int:
    """
    Discover BPGen snippet packs under DevTools/bpgen_snippets/packs.
    """
    try:
        import sots_bpgen_snippets as bpgen_snippets  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import sots_bpgen_snippets: {exc}")
        return 1

    packs = bpgen_snippets.discover_packs()
    print("")
    print(f"Found {len(packs)} BPGen snippet pack(s):")
    for pack in packs:
        pack_name = pack.get("pack_name", "<unknown>")
        version = pack.get("version", "unknown")
        snippets = pack.get("snippets", [])
        snippet_count = len(snippets) if isinstance(snippets, list) else "?"
        path = pack.get("path", "<no path>")
        print(f"- {pack_name} (version {version}, snippets: {snippet_count}) at {path}")
    return 0


def run_bpgen_run_pack(pack_name: str, ue_cmd: str | None) -> int:
    """
    Resolve and run a BPGen snippet pack via Unreal commandlet.
    """
    try:
        import sots_bpgen_snippets as bpgen_snippets  # type: ignore
        import sots_bpgen_runner as bpgen_runner  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import bpgen helpers: {exc}")
        return 1

    packs = bpgen_snippets.discover_packs()
    target = None
    for pack in packs:
        if pack.get("pack_name") == pack_name:
            target = pack
            break

    if not target:
        print(f"[ERROR] Pack '{pack_name}' not found. Available packs: {[p.get('pack_name') for p in packs]}")
        return 1

    log_dir = bpgen_snippets.get_devtools_root() / "bpgen_snippets" / "logs"
    result = bpgen_runner.run_bpgen_snippet_pack(
        pack_path=target.get("path", ""),
        ue_editor_cmd=ue_cmd,
        log_dir=log_dir,
    )

    results = result.get("results", [])
    succeeded = len([r for r in results if r.get("return_code") == 0])
    failed = len(results) - succeeded

    print("")
    print(f"BPGen snippet pack '{target.get('pack_name', pack_name)}' finished:")
    print(f"- {succeeded} snippet(s) succeeded")
    print(f"- {failed} snippet(s) failed (see logs in {log_dir})")
    return 0 if failed == 0 else 1


def run_bpgen_coverage() -> int:
    """
    Print coverage report from snippet_index.json.
    """
    try:
        import sots_bpgen_coverage as coverage  # type: ignore
    except Exception as exc:
        print(f"[ERROR] Could not import sots_bpgen_coverage: {exc}")
        return 1

    data = coverage.load_snippet_index()
    coverage.print_coverage_report(data)
    return 0



# ---------------------------------------------------------------------------
# Main menu (categories)
# ---------------------------------------------------------------------------

def print_main_menu() -> None:
    tools_root = get_tools_root()
    print("")
    print("====================================================")
    print("            SOTS DevTools Python Hub")
    print("----------------------------------------------------")
    print(f" Version : {TOOLBOX_VERSION}")
    print(f" Tools   : {tools_root}")
    print("====================================================")
    print("")
    print("  Main Categories")
    print("  ----------------")
    print("  1) Core maintenance")
    print("  2) FSOTS / architecture")
    print("  3) Plugins & dependencies")
    print("  4) Batch editing / licensing / logs")
    print("  5) High-level project checks")
    print("  6) KEM tools")
    print("")
    print("  0) Exit")
    print("")


# ---------------------------------------------------------------------------
# Category 1: Core maintenance
# ---------------------------------------------------------------------------

def category_core_maintenance() -> None:
    while True:
        print("")
        print("=== Category 1: Core maintenance ===")
        print("")
        print("  1) Clean build caches (+ regen VS project files)")
        print("  2) Run configured build + analyze")
        print("  3) Open project in Unreal Editor")
        print("  4) Delete file/folder by path")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Core> ").strip()

        if choice == "1":
            run_clean_binaries_intermediate()
        elif choice == "2":
            run_build_and_analyze()
        elif choice == "3":
            run_open_unreal_project()
        elif choice == "4":
            run_delete_target()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


# ---------------------------------------------------------------------------
# Category 2: FSOTS / architecture
# ---------------------------------------------------------------------------

def category_fsots_architecture() -> None:
    while True:
        print("")
        print("=== Category 2: FSOTS / architecture ===")
        print("")
        print("  1) Scan FSOTS_* structs (simple summary)")
        print("  2) FSOTS duplicate report")
        print("  3) Architecture lint (laws/config)")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("FSOTS> ").strip()

        if choice == "1":
            run_scan_fsots_structs()
        elif choice == "2":
            run_fsots_duplicate_report()
        elif choice == "3":
            run_architecture_lint()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


# ---------------------------------------------------------------------------
# Category 3: Plugins & dependencies
# ---------------------------------------------------------------------------

def category_plugins_dependencies() -> None:
    while True:
        print("")
        print("=== Category 3: Plugins & dependencies ===")
        print("")
        print("  1) Plugin audit")
        print("  2) Plugin dependency health")
        print("  3) Fix plugin dependencies")
        print("  4) Compare plugin zips")
        print("  5) Package plugin to zip")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Plugins> ").strip()

        if choice == "1":
            run_plugin_audit()
        elif choice == "2":
            run_plugin_dependency_health()
        elif choice == "3":
            run_fix_plugin_dependencies()
        elif choice == "4":
            run_compare_plugin_zips()
        elif choice == "5":
            run_package_plugin()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


# ---------------------------------------------------------------------------
# Category 4: Batch editing / licensing / logs
# ---------------------------------------------------------------------------

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
        print(" 10) Browse ChatGPT inbox (manual SOTS bridge)")
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
            run_apply_latest_chatgpt_inbox()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


# ---------------------------------------------------------------------------
# Category 5: High-level project checks
# ---------------------------------------------------------------------------

def category_high_level_checks() -> None:
    while True:
        print("")
        print("=== Category 5: High-level project checks ===")
        print("")
        print("  1) Project health report")
        print("  2) Ensure plugin modules exist")
        print("  3) (Reserved for future tool)")
        print("  4) [KEM] Execution & Position Report")
        print("  5) [KEM] Callsites Report")
        print("  6) [KEM] Decision Log Analyzer")
        print("  7) [KEM] Coverage Report (from logs)")
        print("  8) [KEM] Anchor Report (source & maps)")
        print("  9) [KEM] OmniTrace Tuning Report")
        print("")
        print("  0) Back to main menu")
        print("")

        choice = input("Checks> ").strip()

        if choice == "1":
            run_project_health_report()
        elif choice == "2":
            run_ensure_plugin_modules()
        elif choice == "3":
            print("[INFO] Reserved slot. No tool wired yet.")
        elif choice == "4":
            run_kem_execution_report()
        elif choice == "5":
            run_kem_callsites_report()
        elif choice == "6":
            run_kem_decision_log_analyzer()
        elif choice == "7":
            run_kem_coverage_report()
        elif choice == "8":
            run_kem_anchor_report()
        elif choice == "9":
            run_kem_omnitrace_tuning_report()
        elif choice in {"0", "b", "B"}:
            break
        else:
            print("[WARN] Unknown option, please try again.")


def category_kem_tools() -> None:
    while True:
        print("")
        print("=== Category 6: KEM tools ===")
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
        print_main_menu()
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
        help="Apply a ChatGPT pack via write_files.py",
    )
    pack_parser.add_argument("pack_path", help="Full path to the pack .txt file")

    pipeline_parser = subparsers.add_parser(
        "run_pipeline",
        help="Run a named pipeline via the pipeline hub",
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

    subparsers.add_parser(
        "bpgen_packs",
        help="List BPGen snippet packs (DevTools/bpgen_snippets/packs)",
    )

    bpgen_run_parser = subparsers.add_parser(
        "bpgen_run_pack",
        help="Run all snippets in a BPGen pack via SOTS_BPGenBuildCommandlet",
    )
    bpgen_run_parser.add_argument("pack_name", help="Pack name to run (from pack_meta.json)")
    bpgen_run_parser.add_argument(
        "--ue",
        dest="ue_cmd",
        help="Path to UnrealEditor-Cmd.exe (overrides default placeholder)",
    )

    subparsers.add_parser(
        "bpgen_coverage",
        help="Print BPGen snippet node coverage report from snippet_index.json",
    )

    snippets_parser = subparsers.add_parser(
        "bep-parkour-snippets-report",
        help="Generate BEP parkour snippet inventory (JSON + Markdown)",
    )
    snippets_parser.add_argument("--zip-path", dest="zip_path", help="Path to BEP_EXPORT_CGF_ParkourComp.zip (or extracted folder)")
    snippets_parser.add_argument("--output-dir", dest="output_dir", help="Output directory for reports (defaults to DevTools/reports)")

    parity_parser = subparsers.add_parser(
        "parkour-parity-sweep",
        help="Run name-based parity sweep between BEP snippets and SOTS_Parkour C++",
    )
    parity_parser.add_argument("--snippets-json", dest="snippets_json", help="Path to bep_parkour_snippets.json")
    parity_parser.add_argument("--plugin-root", dest="plugin_root", help="Root path to SOTS_Parkour plugin")

    return parser


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if not args:
        run_interactive_menu()
        return 0

    parser = build_arg_parser()
    parsed = parser.parse_args(args)

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
    if parsed.command == "bpgen_packs":
        return run_bpgen_packs()
    if parsed.command == "bpgen_run_pack":
        return run_bpgen_run_pack(parsed.pack_name, parsed.ue_cmd)
    if parsed.command == "bpgen_coverage":
        return run_bpgen_coverage()
    if parsed.command == "bep-parkour-snippets-report":
        run_bep_parkour_snippets_report(parsed.zip_path, parsed.output_dir)
        return 0
    if parsed.command == "parkour-parity-sweep":
        run_parkour_parity_sweep(parsed.snippets_json, parsed.plugin_root)
        return 0

    # If we somehow get here, fall back to interactive menu to preserve behavior.
    run_interactive_menu()
    return 0


if __name__ == "__main__":
    sys.exit(main())
