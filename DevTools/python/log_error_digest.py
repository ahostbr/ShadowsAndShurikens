import argparse
import os
import sys
from datetime import datetime

KEYWORDS = [
    "Error:",
    "Fatal:",
    "ensure(",
    "ensureMsgf",
    "check(",
    "checkf(",
    "Assertion failed",
]


def debug_print(msg: str) -> None:
    print(f"[log_error_digest] {msg}")


def ensure_log_dir(path: str) -> str:
    os.makedirs(path, exist_ok=True)
    return path


def write_log(log_dir: str, lines) -> str:
    ensure_log_dir(log_dir)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = os.path.join(log_dir, f"log_error_digest_{ts}.log")
    with open(log_path, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line.rstrip("\n") + "\n")
    return log_path


def find_logs(logs_dir: str, limit: int):
    if not os.path.isdir(logs_dir):
        raise RuntimeError(f"Logs directory does not exist: {logs_dir}")
    entries = []
    for entry in os.listdir(logs_dir):
        if not entry.lower().endswith(".log"):
            continue
        full = os.path.join(logs_dir, entry)
        if not os.path.isfile(full):
            continue
        entries.append((os.path.getmtime(full), full))
    entries.sort(reverse=True)
    return [p for _, p in entries[:limit]]


def extract_key_segment(line: str):
    lowest = None
    for kw in KEYWORDS:
        idx = line.find(kw)
        if idx != -1 and (lowest is None or idx < lowest):
            lowest = idx
    if lowest is None:
        return None
    return line[lowest:].strip()


def digest_logs(log_files, max_len: int = 200):
    summary = {}
    for log in log_files:
        with open(log, "r", encoding="utf-8", errors="ignore") as f:
            for lineno, line in enumerate(f, start=1):
                msg = extract_key_segment(line)
                if msg is None:
                    continue
                key = msg if len(msg) <= max_len else msg[: max_len - 3] + "..."
                if key not in summary:
                    summary[key] = {"count": 1, "first": (log, lineno)}
                else:
                    summary[key]["count"] += 1
    return summary


def format_digest(summary: dict, top: int):
    lines = []
    lines.append("LOG ERROR DIGEST")
    lines.append("-" * 72)
    if not summary:
        lines.append("No error-like messages found.")
        return lines
    items = sorted(summary.items(), key=lambda kv: kv[1]["count"], reverse=True)
    lines.append(f"Top {min(top, len(items))} messages:")
    lines.append("")
    for rank, (msg, data) in enumerate(items[:top], start=1):
        count = data["count"]
        file, lineno = data["first"]
        lines.append(f"{rank}. Count: {count}")
        lines.append(f"   First seen: {file} (line {lineno})")
        lines.append(f"   Message: {msg}")
        lines.append("")
    return lines


def main(argv=None):
    parser = argparse.ArgumentParser(description="Digest UE log errors into a ranked summary.")
    parser.add_argument("--logs-dir", type=str, default=os.path.join("Saved", "Logs"))
    parser.add_argument("--limit", type=int, default=10)
    parser.add_argument("--top", type=int, default=10)
    parser.add_argument("--log-dir", type=str, default=os.path.join("DevTools", "logs"))
    args = parser.parse_args(argv)

    debug_print("Starting log_error_digest")
    debug_print(f"Logs directory: {args.logs_dir}")
    debug_print(f"Log files limit: {args.limit}")
    debug_print(f"Top messages:   {args.top}")
    debug_print(f"Report log dir: {args.log_dir}")

    try:
        files = find_logs(args.logs_dir, args.limit)
    except Exception as exc:
        debug_print(f"FATAL: {exc}")
        return 1
    if not files:
        debug_print("No .log files found.")
        return 0

    debug_print(f"Scanning {len(files)} log file(s).")
    summary = digest_logs(files)
    lines = format_digest(summary, args.top)
    for line in lines:
        print(line)
    report = write_log(args.log_dir, lines)
    debug_print(f"Digest report written to: {report}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
