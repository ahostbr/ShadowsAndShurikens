import os
import sys

import inbox_router
import validate_sots_pack
import log_error_digest
import pack_linter
import pack_template_generator
import devtools_status_dashboard
import report_bundle_exporter
import devtools_selftest

try:
    import run_bpgen_build
except ImportError:  # Keep hub usable even if BPGen helper is missing
    run_bpgen_build = None


def debug_print(msg: str) -> None:
    print(f"[sots_pipeline_hub] {msg}")


def ask(prompt: str, default: str | None = None) -> str:
    if default is not None:
        full = f"{prompt} [{default}]: "
    else:
        full = f"{prompt}: "
    value = input(full).strip()
    if not value and default is not None:
        return default
    return value


def menu_route_inbox():
    debug_print("Selected: Route inbox")
    debug_print(
        "Hint: inbox_router will sort files into <inbox>/<category>/<plugin> "
        "using [SOTS_DEVTOOLS] header metadata."
    )
    inbox_dir = ask("Inbox directory", "chatgpt_inbox")
    dry = ask("Dry run? (y/n)", "n").lower().startswith("y")
    argv = ["--inbox-dir", inbox_dir]
    if dry:
        argv.append("--dry-run")
    rc = inbox_router.main(argv)
    debug_print(f"inbox_router exited with code {rc}")


def menu_validate_pack():
    debug_print("Selected: Validate a single pack")
    pack_path = ask("Path to pack file", "")
    if not pack_path:
        debug_print("No pack path given; aborting.")
        return
    argv = [pack_path]
    rc = validate_sots_pack.main(argv)
    debug_print(f"validate_sots_pack exited with code {rc}")


def menu_lint_packs():
    debug_print("Selected: Lint packs in a folder")
    folder = ask("Folder to scan", "chatgpt_inbox")
    project_root = ask("Project root (optional, blank = none)", "")
    argv = ["--folder", folder]
    if project_root:
        argv.extend(["--project-root", project_root])
    rc = pack_linter.main(argv)
    debug_print(f"pack_linter exited with code {rc}")


def menu_generate_template():
    debug_print("Selected: Generate pack template")
    print("Templates: tag_audit, omnitrace_sweep, kem_execution_audit")
    template = ask("Template name", "tag_audit")
    output_dir = ask("Output directory", "chatgpt_inbox")
    argv = ["--template", template, "--output-dir", output_dir]
    rc = pack_template_generator.main(argv)
    debug_print(f"pack_template_generator exited with code {rc}")


def menu_status_dashboard():
    debug_print("Selected: DevTools status dashboard / update")
    mode = ask("Mode (summary/update)", "summary").lower()
    if mode not in ("summary", "update"):
        debug_print(f"Invalid mode '{mode}', defaulting to 'summary'.")
        mode = "summary"
    argv = ["--mode", mode]
    rc = devtools_status_dashboard.main(argv)
    debug_print(f"devtools_status_dashboard exited with code {rc}")


def menu_export_bundle():
    debug_print("Selected: Export report bundle")
    category = ask("Bundle category (e.g. parkour, stealth, fx)", "global")
    output_dir = ask("Output directory", "DevTools/report_bundles")
    max_lines = ask("Max lines per file (0 = unlimited)", "0")
    sources_raw = ask(
        "Optional source tags (space-separated, leave blank for all)", ""
    )
    try:
        max_lines_int = int(max_lines)
    except ValueError:
        max_lines_int = 0
    sources = []
    if sources_raw.strip():
        sources = sources_raw.split()
    argv = ["--category", category, "--output-dir", output_dir, "--max-lines", str(max_lines_int)]
    if sources:
        argv.extend(["--sources", *sources])
    rc = report_bundle_exporter.main(argv)
    debug_print(f"report_bundle_exporter exited with code {rc}")


def menu_log_error_digest():
    debug_print("Selected: Log error digest")
    logs_dir = ask("Logs directory", os.path.join("Saved", "Logs"))
    limit = ask("How many recent log files?", "10")
    top = ask("How many top messages?", "10")
    argv = ["--logs-dir", logs_dir]
    try:
        argv.extend(["--limit", str(int(limit))])
    except ValueError:
        debug_print(f"Invalid limit '{limit}', ignoring.")
    try:
        argv.extend(["--top", str(int(top))])
    except ValueError:
        debug_print(f"Invalid top '{top}', ignoring.")
    rc = log_error_digest.main(argv)
    debug_print(f"log_error_digest exited with code {rc}")


def menu_run_bpgen_job():
    debug_print("Selected: Run BPGen job (SOTS_BlueprintGen)")
    if run_bpgen_build is None:
        debug_print(
            "ERROR: run_bpgen_build.py not found or failed to import. "
            "Place it next to sots_pipeline_hub.py."
        )
        return
    job_id = ask("Job ID (e.g. BPGEN_HelloWorldTest)")
    if not job_id:
        debug_print("No Job ID specified; aborting.")
        return
    dry = ask(
        "Dry run only (print UE command but do not run)? (y/n)", "n"
    ).lower().startswith("y")
    argv = ["--job-id", job_id]
    if dry:
        argv.append("--dry-run")
    debug_print(f"Calling run_bpgen_build.main with argv={argv}")
    rc = run_bpgen_build.main(argv)
    debug_print(f"run_bpgen_build exited with code {rc}")


def menu_selftest():
    debug_print("Selected: DevTools selftest (health check)")
    mode = ask("Mode (compile/import)", "compile").lower()
    if mode not in ("compile", "import"):
        debug_print(f"Invalid mode '{mode}', defaulting to 'compile'.")
        mode = "compile"
    recursive_ans = ask("Scan subfolders recursively? (y/n)", "n").lower()
    recursive = recursive_ans.startswith("y")
    argv = ["--mode", mode]
    if recursive:
        argv.append("--recursive")
    rc = devtools_selftest.main(argv)
    debug_print(f"devtools_selftest exited with code {rc}")


def main(argv=None):
    debug_print("SOTS Pipeline Hub starting.")
    while True:
        print("")
        print("=== SOTS DevTools Pipeline Hub ===")
        print("  1) Route inbox ([SOTS_DEVTOOLS] files)")
        print("  2) Validate a single pack")
        print("  3) Lint packs in a folder")
        print("  4) Generate a pack template")
        print("  5) DevTools status dashboard / update")
        print("  6) Export a report bundle")
        print("  7) Run log error digest")
        print("  8) Run DevTools selftest (health check)")
        print("  9) Run BPGen job (SOTS_BlueprintGen)")
        print("  0) Exit")
        choice = input("Select an option: ").strip()
        if choice == "1":
            menu_route_inbox()
        elif choice == "2":
            menu_validate_pack()
        elif choice == "3":
            menu_lint_packs()
        elif choice == "4":
            menu_generate_template()
        elif choice == "5":
            menu_status_dashboard()
        elif choice == "6":
            menu_export_bundle()
        elif choice == "7":
            menu_log_error_digest()
        elif choice == "8":
            menu_selftest()
        elif choice == "9":
            menu_run_bpgen_job()
        elif choice == "0":
            debug_print("Exiting SOTS Pipeline Hub.")
            break
        else:
            print("Invalid choice. Try again.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
