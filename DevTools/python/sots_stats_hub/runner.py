from __future__ import annotations

import subprocess
import sys
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional

LogFn = Callable[[str], None]


@dataclass
class ManagedProcess:
    name: str
    process: subprocess.Popen


def default_cwd() -> Path:
    return Path(__file__).resolve().parents[1]


def run_command(args: list[str], log: LogFn, cwd: Optional[Path] = None) -> int:
    log(f"[RUN] {' '.join(args)}")
    proc = subprocess.Popen(args, cwd=str(cwd or default_cwd()), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    assert proc.stdout

    for line in proc.stdout:
        log(line.rstrip())
    proc.wait()
    log(f"[EXIT] code={proc.returncode}")
    return int(proc.returncode)


class ProcessManager:
    def __init__(self, log: LogFn) -> None:
        self.log = log
        self.bridge_proc: Optional[ManagedProcess] = None

    def start_bridge(self, script: Path) -> None:
        if self.bridge_proc and self.bridge_proc.process.poll() is None:
            self.log("[INFO] Bridge server already running.")
            return
        if not script.exists():
            self.log(f"[ERROR] Bridge server script missing: {script}")
            return
        args = [sys.executable, str(script)]
        self.log(f"[START] Bridge server: {' '.join(args)}")
        proc = subprocess.Popen(args, cwd=str(script.parent))
        self.bridge_proc = ManagedProcess(name="bridge", process=proc)
        self.log(f"[INFO] Bridge PID: {proc.pid}")

    def stop_bridge(self) -> None:
        if not self.bridge_proc or self.bridge_proc.process.poll() is not None:
            self.log("[INFO] Bridge server not running.")
            return
        proc = self.bridge_proc.process
        self.log(f"[STOP] Bridge PID {proc.pid}")
        try:
            proc.terminate()
        except Exception as exc:  # noqa: BLE001
            self.log(f"[WARN] Terminate failed: {exc}")

    def run_detached(self, args: list[str]) -> None:
        self.log(f"[LAUNCH] {' '.join(args)}")
        creationflags = 0
        if sys.platform.startswith("win"):
            creationflags = 0x00000008 | 0x00000200
        subprocess.Popen(args, cwd=str(default_cwd()), creationflags=creationflags)

    def run_threaded(self, args: list[str], cwd: Optional[Path] = None, on_finish: Optional[Callable[[int], None]] = None) -> None:
        def _worker() -> None:
            code = run_command(args, self.log, cwd)
            if on_finish:
                try:
                    on_finish(int(code))
                except Exception:
                    pass

        threading.Thread(target=_worker, daemon=True).start()
