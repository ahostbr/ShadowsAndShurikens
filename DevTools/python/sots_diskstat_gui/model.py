from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any


@dataclass
class FileEntry:
    name: str
    path: str
    size: int
    extension: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "path": self.path,
            "size": self.size,
            "extension": self.extension,
        }

    @staticmethod
    def from_dict(payload: dict[str, Any]) -> "FileEntry":
        return FileEntry(
            name=payload.get("name", ""),
            path=payload.get("path", ""),
            size=int(payload.get("size", 0)),
            extension=payload.get("extension", ""),
        )


@dataclass
class Node:
    name: str
    path: str
    size: int
    children: list["Node"] = field(default_factory=list)
    top_files: list[FileEntry] = field(default_factory=list)
    is_dir: bool = True
    parent: "Node | None" = field(default=None, repr=False, compare=False)

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "path": self.path,
            "size": self.size,
            "is_dir": self.is_dir,
            "children": [child.to_dict() for child in self.children],
            "top_files": [f.to_dict() for f in self.top_files],
        }

    @staticmethod
    def from_dict(payload: dict[str, Any]) -> "Node":
        node = Node(
            name=payload.get("name", ""),
            path=payload.get("path", ""),
            size=int(payload.get("size", 0)),
            is_dir=bool(payload.get("is_dir", True)),
        )
        node.children = [Node.from_dict(child) for child in payload.get("children", [])]
        node.top_files = [FileEntry.from_dict(f) for f in payload.get("top_files", [])]
        return node

    def rebuild_parents(self, parent: "Node | None" = None) -> None:
        self.parent = parent
        for child in self.children:
            child.rebuild_parents(self)


@dataclass
class ScanStats:
    total_dirs: int = 0
    total_files: int = 0
    total_errors: int = 0
    total_size: int = 0
    started_at: float = 0.0
    finished_at: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "total_dirs": self.total_dirs,
            "total_files": self.total_files,
            "total_errors": self.total_errors,
            "total_size": self.total_size,
            "started_at": self.started_at,
            "finished_at": self.finished_at,
        }

    @staticmethod
    def from_dict(payload: dict[str, Any]) -> "ScanStats":
        return ScanStats(
            total_dirs=int(payload.get("total_dirs", 0)),
            total_files=int(payload.get("total_files", 0)),
            total_errors=int(payload.get("total_errors", 0)),
            total_size=int(payload.get("total_size", 0)),
            started_at=float(payload.get("started_at", 0.0)),
            finished_at=float(payload.get("finished_at", 0.0)),
        )

