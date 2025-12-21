from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, List, Tuple


@dataclass
class TreemapRect:
    name: str
    path: str
    size: int
    x: float
    y: float
    w: float
    h: float


def _worst(row: list[float], width: float) -> float:
    if not row or width == 0:
        return float("inf")
    max_v = max(row)
    min_v = min(row)
    total = sum(row)
    return max((width**2) * max_v / (total**2), (total**2) / (width**2 * min_v))


def _layout_row(row: list[float], x: float, y: float, w: float, h: float, vertical: bool) -> Tuple[list[Tuple[float, float, float, float]], float, float, float, float]:
    if not row:
        return [], x, y, w, h

    res: list[Tuple[float, float, float, float]] = []
    total = sum(row)

    if vertical:
        row_height = total / w if w else 0
        cx = x
        for size in row:
            rw = size / row_height if row_height else 0
            res.append((cx, y, rw, row_height))
            cx += rw
        y += row_height
        h -= row_height
    else:
        row_width = total / h if h else 0
        cy = y
        for size in row:
            rh = size / row_width if row_width else 0
            res.append((x, cy, row_width, rh))
            cy += rh
        x += row_width
        w -= row_width

    return res, x, y, w, h


def _squarify(
    sizes: list[float],
    row: list[float],
    x: float,
    y: float,
    w: float,
    h: float,
    output: list[Tuple[float, float, float, float]],
) -> list[Tuple[float, float, float, float]]:
    if not sizes:
        layout, *_rest = _layout_row(row, x, y, w, h, w < h)
        output.extend(layout)
        return output

    head = sizes[0]
    new_row = row + [head]
    if _worst(new_row, min(w, h)) <= _worst(row, min(w, h)):
        return _squarify(sizes[1:], new_row, x, y, w, h, output)

    layout, x, y, w, h = _layout_row(row, x, y, w, h, w < h)
    output.extend(layout)
    return _squarify(sizes, [], x, y, w, h, output)


def squarify(sizes: Iterable[int], width: float, height: float) -> list[Tuple[float, float, float, float]]:
    sizes_list = [float(s) for s in sizes if s > 0]
    if not sizes_list or width <= 0 or height <= 0:
        return []

    total_area = width * height
    total_size = sum(sizes_list)
    normalized = [s / total_size * total_area for s in sizes_list]
    return _squarify(normalized, [], 0.0, 0.0, float(width), float(height), [])


def build_treemap(
    items: List[Tuple[str, int, str]],
    width: float,
    height: float,
    padding: float = 2.0,
    gutter: float = 2.0,
) -> list[TreemapRect]:
    """
    Build treemap rectangles for items.

    items: list of (name, size, path)
    """
    filtered = [(n, s, p) for n, s, p in items if s > 0]
    rects: list[TreemapRect] = []
    if not filtered:
        return rects

    sizes = [s for _, s, _ in filtered]
    layout = squarify(sizes, width, height)

    shrink = max(0.0, gutter)
    for (x, y, w, h), (name, size, path) in zip(layout, filtered):
        adj_w = max(1.0, w - shrink)
        adj_h = max(1.0, h - shrink)
        final_w = max(1.0, adj_w - padding)
        final_h = max(1.0, adj_h - padding)
        rects.append(
            TreemapRect(
                name=name,
                path=path,
                size=size,
                x=x + padding / 2 + shrink / 2,
                y=y + padding / 2 + shrink / 2,
                w=final_w,
                h=final_h,
            )
        )
    return rects
