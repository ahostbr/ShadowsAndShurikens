import argparse
import os
import re
import sys
from datetime import datetime

import devtools_header_utils as dhu


"""
inbox_router.py

Routes raw [SOTS_DEVTOOLS] prompt files in `chatgpt_inbox/` into a
more structured folder hierarchy, based on header metadata.

Previous behavior:
    chatgpt_inbox/<file>.txt  ->  chatgpt_inbox/<category>/<plugin>/<file>.txt

New behavior in this pass:
    chatgpt_inbox/<file>.txt  ->  chatgpt_inbox/<category>/<plugin>/<pass>/<file>.txt

Where header keys are:

    category:   high-level bucket (e.g. devtools, plugin_pass, bpgen)
    plugin:     specific plugin or system (e.g. SOTS_BlueprintGen, SOTS_Parkour)
    pass:       pipeline phase (e.g. SPINE, BRIDGE, TOOLS, BUNDLE)
                - If missing, we try `phase`, `pass_type`, then fall back to "GENERAL".

Files without a valid [SOTS_DEVTOOLS] header are left in place and reported
as SKIP entries in the log.

This script is MANUAL USE ONLY, in line with DevTools laws.
"""


def debug_print(msg: str) -> None:
    print(f"[inbox_router] {msg}")


def ensure_log_dir(path: str) -> str:
    os.makedirs(path, exist_ok=True)
    return path


def write_log(log_dir: str, lines) -> str:
    ensure_log_dir(log_dir)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = os.path.join(log_dir, f"inbox_router_{ts}.log")
    with open(log_path, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line.rstrip("\n") + "\n")
    return log_path


def sanitize_segment(value: str, fallback: str) -> str:
    """Sanitize a path segment for use as a folder name.

    - If the value is empty/whitespace, `fallback` is used.
    - Only allow A–Z, a–z, 0–9, underscore, dot, and dash.
    - Everything else becomes an underscore.
    """
    value = (value or "").strip()
    if not value:
        value = fallback
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", value)


def _extract_header_segments(header: dict) -> tuple[str, str]:
    """Extract (plugin, pass) from a parsed header dict.

    We support a few aliases to make prompts more forgiving:

        plugin:    'plugin', 'module'
        pass:      'pass', 'phase', 'pass_type'

    Fallbacks:
        plugin -> 'GLOBAL'
        pass   -> 'GENERAL'
    """

    # Plugin
    raw_plugin = header.get("plugin") or header.get("module")
    plugin = sanitize_segment(raw_plugin, "GLOBAL")

    # Pass / phase
    raw_pass = (
        header.get("pass")
        or header.get("phase")
        or header.get("pass_type")
    )
    pass_segment = sanitize_segment(raw_pass, "GENERAL")
    # Collapse phase suffixes (e.g. SPINE_1, BRIDGE_V2) so everything with the
    # same prefix lands in one folder rather than split by number.
    if "_" in pass_segment:
        pass_segment = pass_segment.split("_", 1)[0]

    return plugin, pass_segment


def route_file(
    file_path: str,
    inbox_dir: str,
    *,
    dry_run: bool = False,
    header_override: dict[str, str] | None = None,
) -> tuple[str, str | None, str]:
    """Route a single file based on its [SOTS_DEVTOOLS] header.

    Returns:
        (old_path, new_path_or_None, reason_str)

    reason_str is used by the caller to classify results:

        - Starts with 'MOVED'  -> counted as moved
        - Starts with 'ERROR'  -> counted as error
        - Anything else        -> counted as skipped
    """
    if header_override is not None:
        header = {k.lower(): str(v) for k, v in header_override.items()}
    else:
        header, err = dhu.load_header_from_file(file_path)
        if header is None:
            return (file_path, None, f"SKIP: {err}")

    plugin, pass_segment = _extract_header_segments(header)
    dest_dir = os.path.join(inbox_dir, plugin, pass_segment)
    os.makedirs(dest_dir, exist_ok=True)

    dest_path = os.path.join(dest_dir, os.path.basename(file_path))

    if dry_run:
        reason = (
            f"DRY-RUN: would move -> {dest_path} "
            f"[plugin={plugin}, pass={pass_segment}]"
        )
        return (file_path, dest_path, reason)

    try:
        os.replace(file_path, dest_path)
        reason = (
            f"MOVED -> {dest_path} "
            f"[plugin={plugin}, pass={pass_segment}]"
        )
        return (file_path, dest_path, reason)
    except Exception as exc:
        return (file_path, None, f"ERROR: move failed: {exc}")


def scan_inbox(inbox_dir: str, dry_run: bool = False):
    """Scan the top-level inbox directory and route text files.

    IMPORTANT:
        - Only files directly under `inbox_dir` are considered.
        - Already-routed files under subfolders (e.g. category/plugin/pass)
          are ignored and left in place.
    """
    if not os.path.isdir(inbox_dir):
        raise RuntimeError(f"Inbox directory does not exist: {inbox_dir}")

    results = []
    for entry in sorted(os.listdir(inbox_dir)):
        full_path = os.path.join(inbox_dir, entry)
        if not os.path.isfile(full_path):
            continue
        if not dhu.is_text_file_name(entry):
            continue
        results.append(route_file(full_path, inbox_dir, dry_run=dry_run))
    return results


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Route [SOTS_DEVTOOLS] inbox files into category/plugin/pass folders."
    )
    parser.add_argument(
        "--inbox-dir",
        type=str,
        default="chatgpt_inbox",
        help="Root inbox directory containing raw [SOTS_DEVTOOLS] prompt files.",
    )
    parser.add_argument(
        "--log-dir",
        type=str,
        default=os.path.join("DevTools", "logs"),
        help="Directory to write inbox_router logs into.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Only print what would happen; do not move any files.",
    )

    args = parser.parse_args(argv)

    debug_print("Starting inbox_router")
    debug_print(f"Inbox directory: {args.inbox_dir}")
    debug_print(f"Log directory:   {args.log_dir}")
    debug_print(f"Dry run:         {args.dry_run}")

    try:
        results = scan_inbox(args.inbox_dir, dry_run=args.dry_run)
    except Exception as exc:
        debug_print(f"FATAL: {exc}")
        # Log the fatal error and exit with non-zero.
        log_lines = [f"FATAL: {exc}"]
        log_path = write_log(args.log_dir, log_lines)
        debug_print(f"Log written to: {log_path}")
        return 1

    log_lines = []

    moved = 0
    skipped = 0
    errors = 0

    for old_path, new_path, reason in results:
        # Keep logs relative to inbox root for readability.
        try:
            rel_old = os.path.relpath(old_path, args.inbox_dir)
        except ValueError:
            rel_old = old_path

        if new_path:
            try:
                rel_new = os.path.relpath(new_path, args.inbox_dir)
            except ValueError:
                rel_new = new_path
            msg = f"{rel_old} -> {rel_new} | {reason}"
        else:
            msg = f"{rel_old} | {reason}"

        log_lines.append(msg)
        debug_print(msg)

        if reason.startswith("MOVED"):
            moved += 1
        elif reason.startswith("ERROR"):
            errors += 1
        else:
            skipped += 1

    summary = (
        f"SUMMARY: moved={moved}, skipped={skipped}, "
        f"errors={errors}, total={len(results)}"
    )
    debug_print(summary)
    log_lines.append(summary)
    log_path = write_log(args.log_dir, log_lines)
    debug_print(f"Log written to: {log_path}")

    return 0 if errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
