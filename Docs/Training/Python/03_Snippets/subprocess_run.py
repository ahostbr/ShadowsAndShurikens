from __future__ import annotations

import subprocess
from dataclasses import dataclass
from typing import Sequence

@dataclass(frozen=True)
class ProcResult:
    cmd: list[str]
    exit_code: int
    stdout: str
    stderr: str

def run(cmd: Sequence[str], *, cwd: str | None = None, timeout_s: int | None = None) -> ProcResult:
    p = subprocess.run(
        list(cmd),
        cwd=cwd,
        timeout=timeout_s,
        capture_output=True,
        text=True,
        check=False,
    )
    return ProcResult(cmd=list(cmd), exit_code=p.returncode, stdout=p.stdout, stderr=p.stderr)

def run_or_raise(cmd: Sequence[str], *, cwd: str | None = None, timeout_s: int | None = None) -> ProcResult:
    r = run(cmd, cwd=cwd, timeout_s=timeout_s)
    if r.exit_code != 0:
        raise RuntimeError(f"Command failed ({r.exit_code}): {' '.join(r.cmd)}\n{r.stderr}")
    return r
