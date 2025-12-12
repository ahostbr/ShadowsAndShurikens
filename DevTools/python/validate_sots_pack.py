import argparse
import os
import sys
from datetime import datetime

import devtools_header_utils as dhu


def debug_print(msg: str) -> None:
    print(f"[validate_sots_pack] {msg}")


def ensure_log_dir(path: str) -> str:
    os.makedirs(path, exist_ok=True)
    return path


def write_log(log_dir: str, lines) -> str:
    ensure_log_dir(log_dir)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = os.path.join(log_dir, f"validate_sots_pack_{ts}.log")
    with open(log_path, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line.rstrip("\n") + "\n")
    return log_path


def validate_pack(file_path: str, project_root: str | None):
    report = []
    fatals = warns = 0
    report.append(f"VALIDATION REPORT for: {file_path}")
    report.append("-" * 72)
    header, err = dhu.load_header_from_file(file_path)
    if header is None:
        msg = f"FATAL: {err}"
        debug_print(msg)
        report.append(msg)
        return report, 1, 0

    report.append("Header fields:")
    for k, v in sorted(header.items()):
        report.append(f"  {k}: {v}")
    report.append("")

    c_f, c_w, c_msgs = dhu.analyze_header_core(header)
    t_f, t_w, t_msgs = dhu.analyze_header_tool_specific(header)
    fatals += c_f + t_f
    warns += c_w + t_w
    report.extend(c_msgs)
    report.extend(t_msgs)

    if project_root is not None:
        report.append("Path checks:")
        p_f, p_w, p_msgs = dhu.analyze_header_paths(header, project_root)
        fatals += p_f
        warns += p_w
        for m in p_msgs:
            report.append("  " + m)
    else:
        report.append("Skipping path checks (no --project-root).")

    if fatals == 0:
        report.append("RESULT: PASS (no fatal errors).")
    else:
        report.append(f"RESULT: FAIL ({fatals} fatal error(s)).")
    if warns:
        report.append(f"Warnings: {warns} (non-fatal).")
    return report, fatals, warns


def main(argv=None):
    parser = argparse.ArgumentParser(description="Validate a [SOTS_DEVTOOLS] pack file.")
    parser.add_argument("--file", required=True)
    parser.add_argument("--project-root", type=str, default=None)
    parser.add_argument(
        "--log-dir",
        type=str,
        default=os.path.join("DevTools", "logs"),
    )
    args = parser.parse_args(argv)

    debug_print("Starting validate_sots_pack")
    debug_print(f"Pack file:    {args.file}")
    debug_print(f"Project root: {args.project_root or '(none)'}")
    debug_print(f"Log directory:{args.log_dir}")

    report, fatals, warns = validate_pack(args.file, args.project_root)
    for line in report:
        print(line)
    log_path = write_log(args.log_dir, report)
    debug_print(f"Log written to: {log_path}")
    return 0 if fatals == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
