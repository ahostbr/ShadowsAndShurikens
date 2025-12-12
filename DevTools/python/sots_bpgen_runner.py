"""
Runner that executes BPGen snippet packs by spawning UnrealEditor-Cmd.exe
with SOTS_BPGenBuildCommandlet.

- No Unreal Python API; pure subprocess + filesystem.
- Always prints what it is doing; logs per-run when log_dir is provided.
"""

from __future__ import annotations

import datetime
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

import sots_bpgen_snippets as bpgen_snippets


DEFAULT_UE_CMD = r"C:/Path/To/UnrealEditor-Cmd.exe"  # TODO: set to your local UE install


def _timestamp() -> str:
    return datetime.datetime.now().strftime("%Y%m%d_%H%M%S")


def _default_uproject() -> Path:
    """
    Try to infer the project .uproject path (one level above DevTools).
    Fallback is a placeholder path.
    """
    devtools_root = Path(__file__).resolve().parent.parent
    candidate = devtools_root.parent / "ShadowsAndShurikens.uproject"
    return candidate if candidate.exists() else Path("PATH/TO/YourProject.uproject")


def _write_log(log_dir: Path, prefix: str, content: str) -> Path:
    log_dir.mkdir(parents=True, exist_ok=True)
    log_path = log_dir / f"{prefix}_{_timestamp()}.log"
    try:
        log_path.write_text(content, encoding="utf-8")
    except Exception as exc:  # pragma: no cover - defensive
        print(f"[WARN] Failed to write log '{log_path}': {exc}", file=sys.stderr)
    return log_path


def run_bpgen_job(
    job_path: str | Path,
    graphspec_path: str | Path | None = None,
    ue_editor_cmd: str | Path | None = None,
    log_dir: str | Path | None = None,
) -> int:
    """
    Run a single BPGen job by invoking UnrealEditor-Cmd.exe with the BPGen commandlet.

    Args:
        job_path: Path to the Job JSON.
        graphspec_path: Optional path to the GraphSpec JSON.
        ue_editor_cmd: Path to UnrealEditor-Cmd.exe. Defaults to DEFAULT_UE_CMD.
        log_dir: Optional directory to write a per-run log.

    Returns:
        The subprocess return code.
    """
    ue_cmd = Path(ue_editor_cmd) if ue_editor_cmd else Path(DEFAULT_UE_CMD)
    uproject_path = _default_uproject()

    cmd: List[str] = [
        str(ue_cmd),
        str(uproject_path),
        "-run=SOTS_BPGenBuildCommandlet",
        f"-JobFile=\"{Path(job_path).resolve()}\"",
        "-unattended",
        "-nop4",
    ]

    if graphspec_path:
        cmd.append(f"-GraphSpecFile=\"{Path(graphspec_path).resolve()}\"")

    print(f"[INFO] Running BPGen job: job={job_path}, graphspec={graphspec_path or '<inline/none>'}")
    print(f"[INFO] Cmd: {' '.join(cmd)}")

    result = subprocess.run(cmd, check=False)
    rc = result.returncode
    print(f"[INFO] BPGen job return code: {rc}")

    if log_dir:
        log_lines = [
            f"Cmd: {' '.join(cmd)}",
            f"JobFile: {Path(job_path).resolve()}",
            f"GraphSpecFile: {Path(graphspec_path).resolve() if graphspec_path else '<inline/none>'}",
            f"ReturnCode: {rc}",
        ]
        _write_log(Path(log_dir), "BPGenRun", "\n".join(log_lines))

    return rc


def run_bpgen_snippet_pack(
    pack_path: str | Path,
    ue_editor_cmd: str | Path | None = None,
    log_dir: str | Path | None = None,
) -> Dict[str, Any]:
    """
    Run all snippets in a pack, returning result summary.
    """
    pack_dir = Path(pack_path).resolve()
    meta = bpgen_snippets.load_pack_meta(pack_dir)
    pack_name = meta.get("pack_name", pack_dir.name) if meta else pack_dir.name

    snippets = bpgen_snippets.discover_snippets_in_pack(pack_dir)
    results: List[Dict[str, Any]] = []

    print(f"[INFO] Running BPGen snippet pack '{pack_name}' from {pack_dir} ...")

    for snippet in snippets:
        snippet_name = snippet.get("snippet_name", "<unknown>")
        job_path = snippet.get("job_path")
        graphspec_path = snippet.get("graphspec_path")

        if not job_path:
            print(f"[WARN] Skipping snippet '{snippet_name}' with missing job_path.")
            continue

        rc = run_bpgen_job(
            job_path=job_path,
            graphspec_path=graphspec_path,
            ue_editor_cmd=ue_editor_cmd,
            log_dir=log_dir,
        )

        results.append(
            {
                "snippet_name": snippet_name,
                "job_path": job_path,
                "graphspec_path": graphspec_path,
                "return_code": rc,
            }
        )

    succeeded = len([r for r in results if r.get("return_code") == 0])
    failed = len(results) - succeeded
    print(f"[INFO] Pack '{pack_name}' finished. Success: {succeeded}, Failures: {failed}")

    return {
        "pack_path": str(pack_dir),
        "pack_name": pack_name,
        "results": results,
    }


if __name__ == "__main__":
    # Simple manual smoke (no arguments) to list packs and show usage hints.
    print("[INFO] sots_bpgen_runner is intended to be called from sots_tools or scripts.")
