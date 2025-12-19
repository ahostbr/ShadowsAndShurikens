import argparse
import json
import os
from datetime import datetime
from pathlib import Path


DEFAULT_INBOX = Path(__file__).resolve().parents[1] / "Inbox" / "BPGenJobs"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Submit a BPGen job JSON into the DevTools inbox.")
    parser.add_argument("action", help="BPGen action name (ignored if --batch is provided)")
    parser.add_argument("params", nargs="?", default="{}", help="JSON string or path to JSON file for params")
    parser.add_argument("--job-id", dest="job_id", default=None, help="Job id (default: bpgen_<timestamp>)")
    parser.add_argument("--batch", dest="batch", default=None, help="JSON string or path to JSON file for batch array")
    parser.add_argument("--allow-apply", dest="allow_apply", action="store_true", help="Enable apply/mutation actions")
    parser.add_argument("--inbox", dest="inbox", default=str(DEFAULT_INBOX), help="Override inbox directory")
    parser.add_argument("--request-id", dest="request_id", default=None, help="Optional request id to echo in reports")
    return parser.parse_args()


def _load_json(value: str):
    path = Path(value)
    if path.exists():
        return json.loads(path.read_text())
    return json.loads(value)


def main() -> None:
    args = parse_args()
    inbox = Path(args.inbox)
    inbox.mkdir(parents=True, exist_ok=True)

    job_id = args.job_id or f"bpgen_{datetime.utcnow().strftime('%Y%m%d_%H%M%S')}"

    payload = {
        "job_id": job_id,
        "allow_apply": bool(args.allow_apply),
    }

    if args.request_id:
        payload["request_id"] = args.request_id

    if args.batch:
        payload["batch"] = _load_json(args.batch)
    else:
        payload["action"] = args.action
        payload["params"] = _load_json(args.params)

    dest = inbox / f"{job_id}.json"
    dest.write_text(json.dumps(payload, indent=2))
    print(f"Wrote job to {dest}")


if __name__ == "__main__":
    main()
