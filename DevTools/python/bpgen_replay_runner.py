"""BPGen replay runner (SPINE_I).

Executes an NDJSON replay against the BPGen TCP bridge and emits deterministic
reports suitable for regression checks.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
from collections import Counter, defaultdict
from typing import Any, Dict, List

from sots_bpgen_bridge_client import bpgen_call, DEFAULT_HOST, DEFAULT_PORT


def load_replay(path: pathlib.Path) -> List[Dict[str, Any]]:
    lines: List[Dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as handle:
        for idx, raw in enumerate(handle, start=1):
            raw = raw.strip()
            if not raw:
                continue
            try:
                lines.append(json.loads(raw))
            except Exception as exc:  # pragma: no cover - defensive
                raise RuntimeError(f"Replay parse failed at line {idx}: {exc}") from exc
    return lines


def summarize_responses(responses: List[Dict[str, Any]]) -> Dict[str, Any]:
    summary: Dict[str, Any] = {
        "total": len(responses),
        "ok": 0,
        "failed": 0,
        "actions": Counter(),
        "error_codes": Counter(),
    }

    for resp in responses:
        action = resp.get("action", "<missing>")
        summary["actions"][action] += 1
        if resp.get("ok"):
            summary["ok"] += 1
        else:
            summary["failed"] += 1
            if resp.get("error_code"):
                summary["error_codes"][resp["error_code"]] += 1

    summary["actions"] = dict(summary["actions"])
    summary["error_codes"] = dict(summary["error_codes"])
    return summary


def diff_reports(expected: Dict[str, Any], actual: Dict[str, Any]) -> str:
    """Return a human-readable diff of key fields."""
    def _key_view(entry: Dict[str, Any]) -> Dict[str, Any]:
        return {
            "action": entry.get("action"),
            "ok": entry.get("ok"),
            "error_code": entry.get("error_code"),
            "errors": entry.get("errors", []),
            "warnings": entry.get("warnings", []),
        }

    expected_resps = expected.get("responses", [])
    actual_resps = actual.get("responses", [])
    lines: List[str] = []
    if len(expected_resps) != len(actual_resps):
        lines.append(f"count mismatch: expected {len(expected_resps)} responses, got {len(actual_resps)}")

    for idx, (exp, act) in enumerate(zip(expected_resps, actual_resps), start=1):
        exp_view = _key_view(exp)
        act_view = _key_view(act)
        if exp_view != act_view:
            lines.append(f"response {idx} differs:\n  expected: {json.dumps(exp_view, sort_keys=True)}\n  actual:   {json.dumps(act_view, sort_keys=True)}")

    if not lines:
        return "no diffs"
    return "\n".join(lines)


def run_replay(
    *,
    host: str,
    port: int,
    replay_path: pathlib.Path,
    out_report: pathlib.Path,
    diff_path: pathlib.Path | None,
    expected_path: pathlib.Path | None,
) -> int:
    replay = load_replay(replay_path)
    responses: List[Dict[str, Any]] = []

    print(f"Running replay with {len(replay)} request(s) -> {replay_path}")
    for idx, request in enumerate(replay, start=1):
        action = request.get("action", "<missing>")
        params = request.get("params", {}) or {}
        params.setdefault("client_protocol_version", "1.0")
        rid = request.get("request_id")
        print(f"[{idx}] {action} ...", end=" ")
        resp = bpgen_call(action, params, host=host, port=port, request_id=rid)
        responses.append(resp)
        status = "ok" if resp.get("ok") else "fail"
        print(status)

    report = {
        "replay": replay_path.name,
        "host": host,
        "port": port,
        "responses": responses,
        "summary": summarize_responses(responses),
    }

    out_report.parent.mkdir(parents=True, exist_ok=True)
    with out_report.open("w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2, sort_keys=True)
        handle.write("\n")
    print(f"Wrote report -> {out_report}")

    if expected_path and expected_path.exists():
        try:
            with expected_path.open("r", encoding="utf-8") as handle:
                expected_report = json.load(handle)
        except Exception as exc:  # pragma: no cover - defensive
            print(f"Failed to load expected report: {exc}")
            expected_report = None

        if expected_report:
            diff_text = diff_reports(expected_report, report)
            if diff_path:
                diff_path.parent.mkdir(parents=True, exist_ok=True)
                with diff_path.open("w", encoding="utf-8") as diff_handle:
                    diff_handle.write(diff_text + "\n")
                print(f"Wrote diff -> {diff_path}")
            print(diff_text)

    return 0


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run a BPGen NDJSON replay against the bridge")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Bridge host (default 127.0.0.1)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Bridge port (default 55557)")
    parser.add_argument("--replay", required=True, type=pathlib.Path, help="Path to NDJSON replay file")
    parser.add_argument("--out", required=True, type=pathlib.Path, help="Path to write JSON report")
    parser.add_argument("--diff", type=pathlib.Path, help="Optional path to write human-readable diff")
    parser.add_argument("--expect", type=pathlib.Path, help="Optional expected report for diffing")
    return parser.parse_args(argv)


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    return run_replay(
        host=args.host,
        port=args.port,
        replay_path=args.replay,
        out_report=args.out,
        diff_path=args.diff,
        expected_path=args.expect,
    )


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
