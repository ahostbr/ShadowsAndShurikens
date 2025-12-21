from __future__ import annotations

from dataclasses import dataclass
from typing import List


@dataclass
class PluginRow:
    name: str
    inbound: int
    outbound: int
    todo: int
    tags: int
    score: int
    pinned: bool = False


@dataclass
class TodoRow:
    plugin: str
    file: str
    line: int
    text: str


@dataclass
class TagRow:
    plugin: str
    file: str
    line: int
    text: str


@dataclass
class ApiRow:
    plugin: str
    file: str
    kind: str
    symbol: str
    header: str


@dataclass
class SearchRow:
    type: str
    plugin: str
    file: str
    preview: str
