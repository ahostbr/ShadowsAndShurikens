import argparse
import os
import sys
from datetime import datetime

import devtools_header_utils as dhu


def debug_print(msg: str) -> None:
    print(f"[pack_linter] {msg}")


def ensure_log_dir(path: str) -> str:
    os.makedirs(path, exist_ok=True)
    return path


def write_log(log_dir: str, lines) -> str:
    ensure_log_dir(log_dir)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = os.path.join(log_dir, f"pack_linter_{ts}.log")
    with open(log_path, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line.rstrip("\n") + "\n")
    return log_path


def iter_candidate_files(inbox_dir: str):
    for entry in sorted(os.listdir(inbox_dir)):
        full = os.path.join(inbox_dir, entry)
        if not os.path.isfile(full):
            continue
        if not dhu.is_text_file_name(entry):
            continue
        yield full


def lint_packs(inbox_dir: str, project_root: str | None):
    rows = []
    summary = {"ok": 0, "warn": 0, "fail": 0, "skipped": 0}
    for path in iter_candidate_files(inbox_dir):
        rel = os.path.relpath(path, os.getcwd())
        header, err = dhu.load_header_from_file(path)
        if header is None:
            msg = f"SKIP: {err}"
            debug_print(f"{rel}: {msg}")
            rows.append({"file": rel, "tool": "-", "status": "SKIP", "details": msg})
            summary["skipped"] += 1
            continue

        status = "OK"
        details = []

        c_f, c_w, c_msgs = dhu.analyze_header_core(header)
        t_f, t_w, t_msgs = dhu.analyze_header_tool_specific(header)
        p_f = p_w = 0
        p_msgs = []
        if project_root is not None:
            p_f, p_w, p_msgs = dhu.analyze_header_paths(header, project_root)

        if c_f + t_f + p_f > 0:
            status = "FAIL"
        elif c_w + t_w + p_w > 0:
            status = "WARN"

        details.extend(c_msgs)
        details.extend(t_msgs)
        details.extend(p_msgs)

        if status == "OK":
            summary["ok"] += 1
            if not details:
                details.append("no issues detected")
        elif status == "WARN":
            summary["warn"] += 1
        else:
            summary["fail"] += 1

        tool = header.get("tool", "").strip() or "?"
        rows.append(
            {
                "file": rel,
                "tool": tool,
                "status": status,
                "details": "; ".join(details) if details else "no issues detected",
            }
        )
    return rows, summary


def format_table(rows, summary):
    lines = []
    lines.append("PACK LINTER SUMMARY")
    lines.append("-" * 72)
    lines.append(
        f"OK={summary['ok']}  WARN={summary['warn']}  FAIL={summary['fail']}  SKIP={summary['skipped']}"
    )
    lines.append("")
    lines.append(f"{'STATUS':8}  {'TOOL':18}  FILE")
    lines.append("-" * 72)
    for row in rows:
        lines.append(f"{row['status']:8}  {row['tool'][:18]:18}  {row['file']}")
        lines.append(f"    -> {row['details']}")
    return lines


def main(argv=None):
    parser = argparse.ArgumentParser(description="Lint multiple [SOTS_DEVTOOLS] packs.")
    parser.add_argument("--inbox-dir", type=str, default="chatgpt_inbox")
    parser.add_argument("--project-root", type=str, default=None)
    parser.add_argument("--log-dir", type=str, default=os.path.join("DevTools", "logs"))
    args = parser.parse_args(argv)

    debug_print("Starting pack_linter")
    debug_print(f"Inbox directory: {args.inbox_dir}")
    debug_print(f"Project root:    {args.project_root or '(none)'}")
    debug_print(f"Log directory:   {args.log_dir}")

    if not os.path.isdir(args.inbox_dir):
        debug_print(f"FATAL: inbox directory does not exist: {args.inbox_dir}")
        return 1

    rows, summary = lint_packs(args.inbox_dir, args.project_root)
    lines = format_table(rows, summary)
    for line in lines:
        print(line)
    log_path = write_log(args.log_dir, lines)
    debug_print(f"Log written to: {log_path}")
    return 0 if summary["fail"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
