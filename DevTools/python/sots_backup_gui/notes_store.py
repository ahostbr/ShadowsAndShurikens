from __future__ import annotations

import json
import os
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


class FileLock:
    def __init__(self, lock_path: Path, timeout_sec: float, logger) -> None:
        self.lock_path = lock_path
        self.timeout_sec = timeout_sec
        self.logger = logger
        self._fh = None

    def __enter__(self):
        self.lock_path.parent.mkdir(parents=True, exist_ok=True)
        self._fh = self.lock_path.open("a+b")
        start = time.monotonic()
        while True:
            try:
                if os.name == "nt":
                    import msvcrt

                    msvcrt.locking(self._fh.fileno(), msvcrt.LK_NBLCK, 1)
                else:
                    import fcntl

                    fcntl.flock(self._fh.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
                if self.logger:
                    self.logger.line(f"[notes] lock acquired: {self.lock_path}")
                return self
            except OSError:
                if time.monotonic() - start > self.timeout_sec:
                    raise TimeoutError(f"Timeout acquiring notes lock: {self.lock_path}")
                time.sleep(0.1)

    def __exit__(self, exc_type, exc, tb):
        if self._fh:
            try:
                if os.name == "nt":
                    import msvcrt

                    msvcrt.locking(self._fh.fileno(), msvcrt.LK_UNLCK, 1)
                else:
                    import fcntl

                    fcntl.flock(self._fh.fileno(), fcntl.LOCK_UN)
                if self.logger:
                    self.logger.line(f"[notes] lock released: {self.lock_path}")
            finally:
                try:
                    self._fh.close()
                except Exception:
                    pass


class NotesStore:
    def __init__(self, notes_path: Path, *, lock_timeout_sec: float = 5.0) -> None:
        self.notes_path = notes_path
        self.lock_timeout_sec = lock_timeout_sec
        self._data = _default_data()

    def load(self, logger) -> dict:
        lock_path = self._lock_path()
        with FileLock(lock_path, self.lock_timeout_sec, logger):
            if not self.notes_path.exists():
                logger.line(f"[notes] Creating notes DB: {self.notes_path}")
                self.notes_path.parent.mkdir(parents=True, exist_ok=True)
                self._data = _default_data()
                _write_json(self.notes_path, self._data)
                return self._data

            try:
                raw = self.notes_path.read_text(encoding="utf-8")
                self._data = _normalize_data(json.loads(raw))
            except Exception as exc:
                logger.line(f"[notes] Failed to load notes JSON: {exc}")
                backup = self.notes_path.with_suffix(self.notes_path.suffix + ".corrupt")
                try:
                    self.notes_path.replace(backup)
                    logger.line(f"[notes] Renamed corrupt notes DB -> {backup}")
                except Exception:
                    logger.line("[notes] Failed to rename corrupt notes DB")
                self._data = _default_data()
                _write_json(self.notes_path, self._data)
            return self._data

    def get(self, snapshot_id: str) -> dict | None:
        notes = self._data.get("notes", {})
        return notes.get(snapshot_id)

    def set_note(self, snapshot_id: str, *, title: str, note: str, logger) -> None:
        title = title.strip()
        note = note.strip()
        lock_path = self._lock_path()
        with FileLock(lock_path, self.lock_timeout_sec, logger):
            self._data = self._reload_locked(logger)
            notes = self._data.setdefault("notes", {})
            if not title and not note:
                if snapshot_id in notes:
                    notes.pop(snapshot_id, None)
                    self._data["updated_at"] = _now_iso()
                    _write_json(self.notes_path, self._data)
                    logger.line(f"[notes] Cleared note for {snapshot_id}")
                return

            entry: dict[str, Any] = {"note": note, "updated_at": _now_iso()}
            if title:
                entry["title"] = title
            notes[snapshot_id] = entry
            self._data["updated_at"] = _now_iso()
            _write_json(self.notes_path, self._data)
            logger.line(f"[notes] Saved note for {snapshot_id}")

    def delete_note(self, snapshot_id: str, logger) -> None:
        lock_path = self._lock_path()
        with FileLock(lock_path, self.lock_timeout_sec, logger):
            self._data = self._reload_locked(logger)
            notes = self._data.setdefault("notes", {})
            if snapshot_id in notes:
                notes.pop(snapshot_id, None)
                self._data["updated_at"] = _now_iso()
                _write_json(self.notes_path, self._data)
                logger.line(f"[notes] Deleted note for {snapshot_id}")

    def all_notes(self) -> dict:
        return self._data.get("notes", {})

    def _reload_locked(self, logger) -> dict:
        if not self.notes_path.exists():
            self._data = _default_data()
            _write_json(self.notes_path, self._data)
            return self._data
        try:
            raw = self.notes_path.read_text(encoding="utf-8")
            self._data = _normalize_data(json.loads(raw))
        except Exception as exc:
            logger.line(f"[notes] Reload failed, keeping in-memory copy: {exc}")
        return self._data

    def _lock_path(self) -> Path:
        return self.notes_path.with_suffix(self.notes_path.suffix + ".lock")


def _now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def _default_data() -> dict:
    return {
        "version": 1,
        "updated_at": _now_iso(),
        "notes": {},
    }


def _normalize_data(data: dict) -> dict:
    if not isinstance(data, dict):
        return _default_data()
    data.setdefault("version", 1)
    data.setdefault("updated_at", _now_iso())
    data.setdefault("notes", {})
    if not isinstance(data["notes"], dict):
        data["notes"] = {}
    return data


def _write_json(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, indent=2), encoding="utf-8", newline="\n")
