from __future__ import annotations

import queue
import sys
import subprocess
import threading
import time
import uuid
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Dict, List, Optional

from PySide6 import QtCore

LogFn = Callable[[str], None]
CallableJob = Callable[[LogFn], int]


@dataclass
class Job:
    id: str
    label: str
    args: Optional[List[str]] = None
    cwd: Optional[Path] = None
    env: Optional[Dict[str, str]] = None
    category: str = "job"
    tool_id: Optional[str] = None
    outputs_hint: List[str] = field(default_factory=list)
    created_ts: float = field(default_factory=time.time)
    started_ts: float = 0.0
    finished_ts: float = 0.0
    state: str = "Queued"  # Queued | Running | Done | Failed | Cancelled
    exit_code: Optional[int] = None
    log_path: Optional[Path] = None
    pid: Optional[int] = None
    func: Optional[CallableJob] = None


class JobManager(QtCore.QObject):
    jobs_changed = QtCore.Signal()
    job_output = QtCore.Signal(str)
    job_finished = QtCore.Signal(Job)

    def __init__(self, project_root: Path, log_dir: Path, log_fn: LogFn) -> None:
        super().__init__()
        self.project_root = project_root
        self.log_dir = log_dir
        self.log_fn = log_fn
        self.queue: "queue.Queue[Job]" = queue.Queue()
        self.queued: List[Job] = []
        self.active: Optional[Job] = None
        self._cancel_event = threading.Event()
        self.history: List[Job] = []
        self._worker_thread = threading.Thread(target=self._worker_loop, daemon=True)
        self._worker_thread.start()

    def enqueue(
        self,
        label: str,
        args: Optional[List[str]] = None,
        *,
        cwd: Optional[Path] = None,
        env: Optional[Dict[str, str]] = None,
        category: str = "job",
        func: Optional[CallableJob] = None,
        tool_id: Optional[str] = None,
        outputs_hint: Optional[List[str]] = None,
    ) -> Job:
        job_id = uuid.uuid4().hex[:8]
        log_path = self._job_log_path(job_id, label, tool_id)
        job = Job(
            id=job_id,
            label=label,
            args=args,
            cwd=cwd,
            env=env,
            category=category,
            log_path=log_path,
            func=func,
            tool_id=tool_id,
            outputs_hint=outputs_hint or [],
        )
        self.queue.put(job)
        self.queued.append(job)
        self.jobs_changed.emit()
        prefix = f"[TOOL:{tool_id}] " if tool_id else ""
        self.log_fn(f"[JOB] {prefix}queued {label} ({job_id})")
        return job

    def cancel_active(self) -> None:
        if self.active and self.active.state == "Running":
            self._cancel_event.set()
            self.log_fn(f"[JOB] cancel requested for {self.active.label}")

    def clear_history(self) -> None:
        self.history = []
        self.jobs_changed.emit()

    def _worker_loop(self) -> None:
        while True:
            job = self.queue.get()
            if job in self.queued:
                self.queued.remove(job)
            self.active = job
            self._cancel_event.clear()
            job.state = "Running"
            job.started_ts = time.time()
            self.jobs_changed.emit()
            try:
                code = self._run_job(job)
                job.exit_code = code
                job.state = "Done" if code == 0 and not self._cancel_event.is_set() else ("Cancelled" if self._cancel_event.is_set() else "Failed")
            except Exception as exc:  # noqa: BLE001
                job.exit_code = -1
                job.state = "Failed"
                self.log_fn(f"[ERR] job {job.label} failed: {exc}")
            job.finished_ts = time.time()
            self.history.append(job)
            self.active = None
            self.jobs_changed.emit()
            self.job_finished.emit(job)
            self.queue.task_done()

    def _run_job(self, job: Job) -> int:
        prefix = f"[TOOL:{job.tool_id}] " if job.tool_id else ""
        self.log_fn(f"[JOB] {prefix}start {job.label} ({job.id})")
        if job.func:
            return int(job.func(self._log_to_job(job)))
        return self._run_process(job)

    def _run_process(self, job: Job) -> int:
        if not job.args:
            self.log_fn(f"[ERR] job {job.label} missing args")
            return -1
        job.log_path.parent.mkdir(parents=True, exist_ok=True)
        with job.log_path.open("w", encoding="utf-8") as lf:
            proc = subprocess.Popen(
                job.args,
                cwd=str(job.cwd or self.project_root),
                env={**job.env} if job.env else None,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                creationflags=0x00000008 | 0x00000200 if sys.platform.startswith("win") else 0,  # type: ignore
            )
            job.pid = proc.pid
            for line in proc.stdout or []:
                if self._cancel_event.is_set():
                    try:
                        proc.terminate()
                    except Exception:
                        pass
                msg = line.rstrip()
                lf.write(msg + "\n")
                lf.flush()
                self.job_output.emit(msg)
                prefix = f"[TOOL:{job.tool_id}] " if job.tool_id else ""
                self.log_fn(f"[JOB] {prefix}{job.label}: {msg}")
            proc.wait()
            return int(proc.returncode)

    def _log_to_job(self, job: Job) -> LogFn:
        def inner(msg: str) -> None:
            try:
                if job.log_path:
                    job.log_path.parent.mkdir(parents=True, exist_ok=True)
                    with job.log_path.open("a", encoding="utf-8") as lf:
                        lf.write(msg + "\n")
            except Exception:
                pass
            self.job_output.emit(msg)
            prefix = f"[TOOL:{job.tool_id}] " if job.tool_id else ""
            self.log_fn(f"[JOB] {prefix}{job.label}: {msg}")

        return inner

    def _job_log_path(self, job_id: str, label: str, tool_id: Optional[str]) -> Path:
        safe_label = "".join(ch if ch.isalnum() else "_" for ch in label)[:40]
        safe_tool = f"{tool_id}_" if tool_id else ""
        ts = time.strftime("%Y%m%d_%H%M%S")
        return self.log_dir / "JobLogs" / f"{ts}_{safe_tool}{safe_label}_{job_id}.log"
