import argparse
import json
from pathlib import Path


DEFAULT_REPORTS = Path(__file__).resolve().parents[1] / "Reports" / "BPGen"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarize BPGen reports in the DevTools reports folder.")
    parser.add_argument("--reports", dest="reports", default=str(DEFAULT_REPORTS), help="Override reports directory")
    parser.add_argument("--limit", dest="limit", type=int, default=20, help="Max reports to list")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    reports_dir = Path(args.reports)
    if not reports_dir.exists():
        print(f"Reports directory not found: {reports_dir}")
        return

    reports = sorted(reports_dir.glob("*.json"), reverse=True)
    if not reports:
        print("No reports found")
        return

    for path in reports[: args.limit]:
        try:
            data = json.loads(path.read_text())
        except Exception as exc:  # noqa: BLE001
            print(f"{path.name}: failed to parse ({exc})")
            continue

        status = data.get("status") or data.get("result") or "unknown"
        request_id = data.get("request_id")
        actions = data.get("actions") or data.get("batch") or []
        action_names = ",".join([a.get("action", "?") for a in actions]) if isinstance(actions, list) else ""
        summary = data.get("summary") or data.get("error") or ""
        print(f"{path.name}: status={status} actions=[{action_names}] request_id={request_id} {summary}")


if __name__ == "__main__":
    main()
