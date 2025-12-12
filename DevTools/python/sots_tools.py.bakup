from __future__ import annotations

import sys
import subprocess
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
    Manual gateway:

    - Finds the newest file in chatgpt_inbox.
    - Calls sots_chatgpt_dispatcher.dispatch_file(latest, force=True),
      so mode: manual headers actually execute.
    """
    files = list_chatgpt_inbox(limit=1)
    if not files:
        return

    latest = files[0]
    print(f"[INFO] Applying latest ChatGPT inbox message: {latest}")

    try:
        from sots_chatgpt_dispatcher import dispatch_file  # type: ignore
    except Exception as exc:
        print("[ERROR] Could not import sots_chatgpt_dispatcher.dispatch_file.")
        print(f"       {exc}")
        print("       Make sure sots_chatgpt_dispatcher.py defines:")
        print("           def dispatch_file(prompt_path: Path, *, force: bool = False) -> None")
        return

    # Force = True ignores mode: manual and actually runs the tool.
    dispatch_file(latest, force=True)
    print("[INFO] Manual dispatcher call completed (force=True).")


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
        print(" 10) Apply latest ChatGPT inbox message (manual SOTS bridge)")
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

def main() -> None:
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


if __name__ == "__main__":
    main()
