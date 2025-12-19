"""
Poll a BPGen jobs directory and submit each job as a batch to the bridge.

Job file format (JSON):
{
  "job_id": "optional",
  "created_utc": "...",
  "commands": [ ... ],
  "atomic": true,
  "stop_on_error": true,
  "session_id": "optional"
}
"""

from __future__ import annotations

import argparse
import datetime
import json
import shutil
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Tuple

from sots_bpgen_bridge_client import DEFAULT_HOST, DEFAULT_PORT
from bpgen_batch_submit import submit_batch, summarize_response


def _utc_iso() -> str:
    return datetime.datetime.utcnow().replace(microsecond=0).isoformat() + "Z"


def _timestamp() -> str:
    return datetime.datetime.utcnow().strftime("%Y%m%d_%H%M%S")


def _devtools_root() -> Path:
    return Path(__file__).resolve().parent.parent


def _default_jobs_dir() -> Path:
    return _devtools_root() / "Inbox" / "BPGenJobs"


def _default_report_dir() -> Path:
    return _devtools_root() / "reports" / "BPGen"


def discover_jobs(jobs_dir: Path) -> List[Path]:
    if not jobs_dir.exists():
        return []
    return sorted([path for path in jobs_dir.glob("*.json") if path.is_file()])


def load_job(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError("Job file must contain a JSON object.")
    return data


def write_report(report_path: Path, report: Dict[str, Any]) -> None:
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2, sort_keys=True)
        handle.write("\n")


def write_log(log_path: Path, lines: List[str]) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")


def move_job(job_path: Path, dest_dir: Path) -> Path:
    dest_dir.mkdir(parents=True, exist_ok=True)
    target = dest_dir / job_path.name
    if target.exists():
        target = dest_dir / f"{job_path.stem}_{_timestamp()}{job_path.suffix}"
    return Path(shutil.move(str(job_path), str(target)))


def process_job(
    job_path: Path,
    *,
    host: str,
    port: int,
    report_dir: Path,
) -> Tuple[bool, Path, Path]:
    job_data = load_job(job_path)
    job_id = str(job_data.get("job_id") or job_data.get("batch_id") or job_path.stem)

    action, params, response = submit_batch(
        batch_data=job_data,
        host=host,
        port=port,
        request_id=job_id,
    )

    summary = summarize_response(response)
    report_path = report_dir / f"{job_id}_report.json"
    log_path = report_dir / f"{job_id}_log.txt"

    report = {
        "job_id": job_id,
        "job_file": str(job_path.resolve()),
        "submitted_utc": _utc_iso(),
        "action": action,
        "host": host,
        "port": port,
        "params": {
            "batch_id": params.get("batch_id"),
            "atomic": params.get("atomic"),
            "stop_on_error": params.get("stop_on_error"),
            "session_id": params.get("session_id"),
            "command_count": len(params.get("commands", []) or []),
        },
        "summary": summary,
        "response": response,
    }

    write_report(report_path, report)

    log_lines = [
        f"JobId: {job_id}",
        f"JobFile: {job_path.resolve()}",
        f"Action: {action}",
        f"Host: {host}",
        f"Port: {port}",
        f"Ok: {summary.get('ok')}",
        f"Steps: {summary.get('steps')}",
        f"OkSteps: {summary.get('ok_steps')}",
        f"FailedSteps: {summary.get('failed_steps')}",
        f"MsTotal: {summary.get('ms_total')}",
        f"ErrorCode: {summary.get('error_code')}",
        f"Report: {report_path}",
    ]
    write_log(log_path, log_lines)

    return bool(summary.get("ok")), report_path, log_path


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Poll a BPGen jobs directory and run batches.")
    parser.add_argument("--jobs-dir", type=Path, default=_default_jobs_dir(), help="Jobs directory to poll.")
    parser.add_argument("--report-dir", type=Path, default=_default_report_dir(), help="Directory for reports/logs.")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Bridge host (default 127.0.0.1).")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Bridge port (default 55557).")
    parser.add_argument("--interval", type=float, default=3.0, help="Poll interval in seconds.")
    parser.add_argument("--once", action="store_true", help="Process current jobs and exit.")
    return parser.parse_args(argv)


def run_loop(args: argparse.Namespace) -> int:
    jobs_dir: Path = args.jobs_dir
    report_dir: Path = args.report_dir
    done_dir = jobs_dir / "Done"
    failed_dir = jobs_dir / "Failed"

    jobs_dir.mkdir(parents=True, exist_ok=True)
    report_dir.mkdir(parents=True, exist_ok=True)

    print(f"[INFO] BPGen jobs runner starting. JobsDir={jobs_dir} ReportDir={report_dir}")
    had_failures = False

    while True:
        jobs = discover_jobs(jobs_dir)
        if not jobs:
            print(f"[INFO] No jobs found. Sleeping {args.interval:.1f}s...")
            if args.once:
                break
            time.sleep(args.interval)
            continue

        for job_path in jobs:
            print(f"[INFO] Processing job: {job_path.name}")
            try:
                ok, report_path, log_path = process_job(
                    job_path,
                    host=args.host,
                    port=args.port,
                    report_dir=report_dir,
                )
                dest = move_job(job_path, done_dir if ok else failed_dir)
                print(f"[INFO] Job {'OK' if ok else 'FAILED'} -> report={report_path.name} log={log_path.name} archive={dest.name}")
                if not ok:
                    had_failures = True
            except Exception as exc:
                had_failures = True
                error_report = report_dir / f"{job_path.stem}_error_{_timestamp()}.txt"
                write_log(error_report, [f"Job failed: {job_path}", f"Error: {exc}"])
                dest = move_job(job_path, failed_dir)
                print(f"[ERROR] Job failed: {job_path.name} -> {exc}")
                print(f"[INFO] Archived failed job to {dest} and wrote {error_report}")

        if args.once:
            break

    return 1 if had_failures and args.once else 0


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    return run_loop(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
