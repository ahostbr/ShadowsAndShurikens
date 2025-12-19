"""Upgrade a BPGen GraphSpec via canonicalize_spec (SPINE_M).

Writes:
  - <name>_upgraded.json
  - <name>_upgrade_report.json
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import sys
from typing import Any, Dict, List

from sots_bpgen_bridge_client import bpgen_call, DEFAULT_HOST, DEFAULT_PORT


def _load_json(path: pathlib.Path) -> Dict[str, Any]:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except Exception as exc:  # pragma: no cover - defensive
        raise RuntimeError(f"Failed to read JSON from {path}: {exc}") from exc


def _write_json(path: pathlib.Path, payload: Dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, sort_keys=True)
        handle.write("\n")


def upgrade_spec(
    *,
    spec_path: pathlib.Path,
    out_dir: pathlib.Path,
    host: str,
    port: int,
    auth_token: str | None,
) -> int:
    spec = _load_json(spec_path)
    params: Dict[str, Any] = {"graph_spec": spec}
    if auth_token:
        params["auth_token"] = auth_token

    resp = bpgen_call("canonicalize_spec", params, host=host, port=port)
    if not resp.get("ok"):
        errors = resp.get("errors", [])
        msg = errors[0] if errors else "canonicalize_spec failed"
        print(f"[ERROR] {msg}")
        return 1

    result = resp.get("result", {}) or {}
    canonical_spec = result.get("canonical_spec")
    if not canonical_spec:
        print("[ERROR] canonicalize_spec did not return canonical_spec")
        return 1

    stem = spec_path.stem
    upgraded_path = out_dir / f"{stem}_upgraded.json"
    report_path = out_dir / f"{stem}_upgrade_report.json"

    _write_json(upgraded_path, canonical_spec)

    report = {
        "input_spec": str(spec_path),
        "output_spec": str(upgraded_path),
        "spec_migrated": bool(result.get("spec_migrated", False)),
        "diff_notes": result.get("diff_notes", []),
        "migration_notes": result.get("migration_notes", []),
        "warnings": resp.get("warnings", []),
    }
    _write_json(report_path, report)

    print(f"Wrote upgraded spec -> {upgraded_path}")
    print(f"Wrote upgrade report -> {report_path}")
    print(f"Spec migrated: {report['spec_migrated']}")
    if report["diff_notes"]:
        print(f"Diff notes: {len(report['diff_notes'])}")
    if report["migration_notes"]:
        print(f"Migration notes: {len(report['migration_notes'])}")
    return 0


def parse_args(argv: List[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Canonicalize/upgrade a BPGen GraphSpec JSON file")
    parser.add_argument("--spec", required=True, type=pathlib.Path, help="Path to GraphSpec JSON file")
    parser.add_argument("--out-dir", type=pathlib.Path, help="Optional output directory (defaults to spec folder)")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Bridge host (default 127.0.0.1)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Bridge port (default 55557)")
    parser.add_argument("--auth-token", type=str, help="Optional auth token (overrides SOTS_BPGEN_AUTH_TOKEN)")
    return parser.parse_args(argv)


def main(argv: List[str]) -> int:
    args = parse_args(argv)
    out_dir = args.out_dir or args.spec.parent
    auth_token = args.auth_token or os.environ.get("SOTS_BPGEN_AUTH_TOKEN")
    return upgrade_spec(
        spec_path=args.spec,
        out_dir=out_dir,
        host=args.host,
        port=args.port,
        auth_token=auth_token,
    )


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
