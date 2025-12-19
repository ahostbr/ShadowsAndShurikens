"""
Submit a BPGen batch request to the TCP bridge and write a report.

Batch file format (JSON):
{
  "job_id": "optional",
  "batch_id": "optional",
  "atomic": true,
  "stop_on_error": true,
  "session_id": "optional",
  "commands": [
    {"action": "discover_nodes", "params": {...}},
    {"action": "apply_graph_spec", "params": {...}}
  ]
}
"""

from __future__ import annotations

import argparse
import datetime
import json
import sys
from pathlib import Path
from typing import Any, Dict, Tuple

from sots_bpgen_bridge_client import bpgen_call, DEFAULT_HOST, DEFAULT_PORT


def _utc_timestamp() -> str:
    return datetime.datetime.utcnow().strftime("%Y%m%d_%H%M%S")


def _utc_iso() -> str:
    return datetime.datetime.utcnow().replace(microsecond=0).isoformat() + "Z"


def _devtools_root() -> Path:
    return Path(__file__).resolve().parent.parent


def _default_report_dir() -> Path:
    return _devtools_root() / "reports" / "BPGen"


def load_batch_file(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError("Batch file must contain a JSON object.")
    return data


def build_batch_params(batch_data: Dict[str, Any], session_id_override: str | None) -> Dict[str, Any]:
    params_source = batch_data.get("params") if isinstance(batch_data.get("params"), dict) else batch_data

    params: Dict[str, Any] = {}
    if "commands" in params_source:
        params["commands"] = params_source["commands"]

    if "batch_id" in params_source:
        params["batch_id"] = params_source["batch_id"]
    elif "job_id" in batch_data:
        params["batch_id"] = batch_data["job_id"]

    for key in ("atomic", "stop_on_error", "session_id"):
        if key in params_source:
            params[key] = params_source[key]

    if session_id_override:
        params["session_id"] = session_id_override

    params.setdefault("atomic", True)
    params.setdefault("stop_on_error", True)
    params.setdefault("client_protocol_version", "1.0")

    commands = params.get("commands")
    if not isinstance(commands, list) or not commands:
        raise ValueError("Batch params must include a non-empty commands array.")

    return params


def determine_action(params: Dict[str, Any], action_override: str | None) -> str:
    if action_override:
        return action_override
    if params.get("session_id"):
        return "session_batch"
    return "batch"


def submit_batch(
    *,
    batch_data: Dict[str, Any],
    host: str,
    port: int,
    session_id_override: str | None = None,
    action_override: str | None = None,
    request_id: str | None = None,
) -> Tuple[str, Dict[str, Any], Dict[str, Any]]:
    params = build_batch_params(batch_data, session_id_override)
    action = determine_action(params, action_override)
    response = bpgen_call(action, params, host=host, port=port, request_id=request_id)
    return action, params, response


def summarize_response(response: Dict[str, Any]) -> Dict[str, Any]:
    result = response.get("result", {}) if isinstance(response.get("result"), dict) else {}
    summary = result.get("summary", {}) if isinstance(result.get("summary"), dict) else {}
    return {
        "ok": bool(response.get("ok")),
        "batch_id": result.get("batch_id"),
        "steps": len(result.get("steps", []) or []),
        "ok_steps": summary.get("ok_steps"),
        "failed_steps": summary.get("failed_steps"),
        "error_codes": summary.get("error_codes", {}),
        "ms_total": result.get("ms_total"),
        "error_code": response.get("error_code"),
    }


def write_report(report_path: Path, report: Dict[str, Any]) -> None:
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2, sort_keys=True)
        handle.write("\n")


def resolve_report_path(batch_path: Path, batch_id: str | None, out_path: Path | None) -> Path:
    if out_path:
        return out_path

    report_dir = _default_report_dir()
    base = batch_id or batch_path.stem
    candidate = report_dir / f"{base}_report.json"
    if not candidate.exists():
        return candidate
    return report_dir / f"{base}_{_utc_timestamp()}_report.json"


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Submit a BPGen batch request to the bridge.")
    parser.add_argument("--batch", required=True, type=Path, help="Path to batch JSON file.")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Bridge host (default 127.0.0.1).")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Bridge port (default 55557).")
    parser.add_argument("--session-id", help="Optional session_id override (forces session_batch).")
    parser.add_argument("--action", choices=["batch", "session_batch"], help="Override action type.")
    parser.add_argument("--out", type=Path, help="Optional report output path.")
    parser.add_argument("--request-id", help="Optional request_id to use for the bridge call.")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    batch_path: Path = args.batch
    if not batch_path.exists():
        print(f"[ERROR] Batch file not found: {batch_path}")
        return 1

    try:
        batch_data = load_batch_file(batch_path)
    except Exception as exc:
        print(f"[ERROR] Failed to read batch file: {exc}")
        return 1

    print(f"[INFO] Submitting batch: {batch_path}")
    try:
        action, params, response = submit_batch(
            batch_data=batch_data,
            host=args.host,
            port=args.port,
            session_id_override=args.session_id,
            action_override=args.action,
            request_id=args.request_id,
        )
    except Exception as exc:
        print(f"[ERROR] Failed to submit batch: {exc}")
        return 1

    summary = summarize_response(response)
    report = {
        "batch_file": str(batch_path.resolve()),
        "submitted_utc": _utc_iso(),
        "host": args.host,
        "port": args.port,
        "action": action,
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

    report_path = resolve_report_path(batch_path, summary.get("batch_id"), args.out)
    write_report(report_path, report)

    print(f"[INFO] Batch action: {action}")
    print(f"[INFO] Result: ok={summary.get('ok')} steps={summary.get('steps')} ok_steps={summary.get('ok_steps')} failed_steps={summary.get('failed_steps')}")
    print(f"[INFO] Report written: {report_path}")

    return 0 if summary.get("ok") else 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
