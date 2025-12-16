from __future__ import annotations

import argparse
import datetime
import os
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple, Iterable


ANCHOR_START = "[CONTEXT_ANCHOR]"
ANCHOR_END = "[/CONTEXT_ANCHOR]"

LOCK_RE = re.compile(r"\bLock_([A-Za-z0-9_]+)\b")
PLUGIN_LINE_RE = re.compile(r"^\s*plugin\s*:\s*([A-Za-z0-9_]+)\s*$", re.IGNORECASE | re.MULTILINE)
PLUGIN_PATH_RE = re.compile(r"Plugins[\\/]+([A-Za-z0-9_]+)[\\/]+")


def now_iso() -> str:
    return datetime.datetime.now().isoformat(timespec="seconds")


def ensure_dir(p: Path) -> None:
    p.mkdir(parents=True, exist_ok=True)


def write_log(log_path: Path, line: str) -> None:
    ensure_dir(log_path.parent)
    with log_path.open("a", encoding="utf-8") as f:
        f.write(f"[{now_iso()}] {line}\n")


@dataclass
class RouteResult:
    src: Path
    plugins: List[str]
    dests: List[Path]
    action: str
    reason: str


def extract_anchor_block(text: str) -> Optional[str]:
    start = text.find(ANCHOR_START)
    if start < 0:
        return None
    end = text.find(ANCHOR_END, start)
    if end < 0:
        return None
    return text[start : end + len(ANCHOR_END)]


def infer_plugins_from_block(block: str) -> Tuple[List[str], str]:
    """
    Returns (plugins, reason).
    """
    # 1) Lock_<PLUGIN> tokens
    locks = LOCK_RE.findall(block)
    if locks:
        plugins = sorted(set(locks))
        return plugins, "Lock_<PLUGIN> tokens found in [CONTEXT_ANCHOR]"

    # 2) explicit plugin: line
    m = PLUGIN_LINE_RE.search(block)
    if m:
        plugins = [m.group(1).strip()]
        return plugins, "Explicit plugin: line found in [CONTEXT_ANCHOR]"

    # 3) Plugins/<PluginName>/ paths
    paths = PLUGIN_PATH_RE.findall(block)
    if paths:
        plugins = sorted(set(paths))
        return plugins, "Plugins/<PluginName>/ path found in [CONTEXT_ANCHOR]"

    return [], "No plugin could be inferred from [CONTEXT_ANCHOR]"


def safe_dest_name(src_name: str, plugin: Optional[str], multi: bool) -> str:
    """
    If we’re routing the same file to multiple plugins, suffix it to avoid collisions.
    """
    if not plugin or not multi:
        return src_name
    p = Path(src_name)
    return f"{p.stem}__{plugin}{p.suffix}"


def uniquify_if_exists(dest: Path) -> Path:
    if not dest.exists():
        return dest
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    return dest.with_name(f"{dest.stem}__{stamp}{dest.suffix}")


def resolve_inbox_default(project_root: Path) -> Path:
    # Most common layout:
    #   <ProjectRoot>/DevTools/python/chatgpt_inbox
    return project_root / "DevTools" / "python" / "chatgpt_inbox"


def resolve_log_default(project_root: Path) -> Path:
    # Prefer DevTools reports if it exists, else fall back to Docs/Anchor
    dev_reports = project_root / "DevTools" / "python" / "reports"
    if dev_reports.exists():
        return dev_reports / "context_anchor_route.log"
    return project_root / "Docs" / "Anchor" / "context_anchor_route.log"


def iter_anchor_files(inbox_root: Path, pattern: str) -> Iterable[Path]:
    # pattern is glob-style, e.g. SESSION_ANCHOR_*.md
    yield from inbox_root.rglob(pattern)


def route_one(
    src: Path,
    project_root: Path,
    dry_run: bool,
    copy_instead_of_move: bool,
    fallback_dir: Path,
) -> RouteResult:
    text = src.read_text(encoding="utf-8", errors="ignore")
    block = extract_anchor_block(text)

    if not block:
        return RouteResult(src=src, plugins=[], dests=[], action="skip", reason="No [CONTEXT_ANCHOR] block found")

    plugins, reason = infer_plugins_from_block(block)
    multi = len(plugins) > 1

    dests: List[Path] = []
    if plugins:
        for plugin in plugins:
            dest_dir = project_root / "Plugins" / plugin / "Docs" / "Anchor"
            dests.append(dest_dir / safe_dest_name(src.name, plugin, multi))
    else:
        dests.append((project_root / "Docs" / "Anchor") / src.name)

    # Resolve collisions deterministically per dest.
    dests = [uniquify_if_exists(d) for d in dests]

    action = "copy" if copy_instead_of_move else "move"

    if dry_run:
        return RouteResult(src=src, plugins=plugins, dests=dests, action=f"dry-run:{action}", reason=reason)

    if copy_instead_of_move:
        for dest in dests:
            ensure_dir(dest.parent)
            shutil.copy2(src, dest)
        return RouteResult(src=src, plugins=plugins, dests=dests, action="copy", reason=reason)

    # move semantics
    if len(dests) == 1:
        dest = dests[0]
        ensure_dir(dest.parent)
        shutil.move(str(src), str(dest))
        return RouteResult(src=src, plugins=plugins, dests=dests, action="move", reason=reason)

    # Multiple destinations: copy to all but last, then move to last.
    for dest in dests[:-1]:
        ensure_dir(dest.parent)
        shutil.copy2(src, dest)

    last = dests[-1]
    ensure_dir(last.parent)
    shutil.move(str(src), str(last))

    return RouteResult(src=src, plugins=plugins, dests=dests, action="move", reason=reason)


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Route Context Anchor files to per-plugin Docs/Anchor folders.")
    p.add_argument("--project-root", type=str, default=os.environ.get("PROJECT_ROOT", ""), help="Path to UE project root (contains Plugins/).")
    p.add_argument("--inbox-root", type=str, default="", help="Path to chatgpt_inbox root. Default: <project-root>/DevTools/python/chatgpt_inbox")
    p.add_argument("--pattern", type=str, default="SESSION_ANCHOR_*.md", help="Glob pattern to find anchor files.")
    p.add_argument("--apply", action="store_true", help="Actually move/copy files (default is dry-run).")
    p.add_argument("--copy", action="store_true", help="Copy instead of move (move is default).")
    p.add_argument("--fallback-dir", type=str, default="", help="Fallback directory when plugin can’t be inferred. Default: <project-root>/Docs/Anchor")
    p.add_argument("--log", type=str, default="", help="Optional explicit log file path.")
    return p.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)

    if not args.project_root:
        print("[ERROR] --project-root is required (or set env PROJECT_ROOT).")
        return 2

    project_root = Path(args.project_root).resolve()
    inbox_root = Path(args.inbox_root).resolve() if args.inbox_root else resolve_inbox_default(project_root)
    fallback_dir = Path(args.fallback_dir).resolve() if args.fallback_dir else (project_root / "Docs" / "Anchor")
    log_path = Path(args.log).resolve() if args.log else resolve_log_default(project_root)

    dry_run = not args.apply
    copy_instead_of_move = bool(args.copy)

    print("[INFO] Context Anchor Router")
    print(f"- Project root: {project_root}")
    print(f"- Inbox root:   {inbox_root}")
    print(f"- Pattern:      {args.pattern}")
    print(f"- Mode:         {'DRY-RUN' if dry_run else ('COPY' if copy_instead_of_move else 'MOVE')}")
    print(f"- Fallback:     {fallback_dir}")
    print(f"- Log:          {log_path}")
    print("")

    if not inbox_root.exists():
        print(f"[WARN] Inbox root does not exist: {inbox_root}")
        write_log(log_path, f"WARN inbox root missing: {inbox_root}")
        return 0

    files = list(iter_anchor_files(inbox_root, args.pattern))
    if not files:
        print("[RESULT] No matching anchor files found.")
        write_log(log_path, "No matching anchor files found.")
        return 0

    routed = 0
    skipped = 0

    for f in sorted(files):
        try:
            res = route_one(
                src=f,
                project_root=project_root,
                dry_run=dry_run,
                copy_instead_of_move=copy_instead_of_move,
                fallback_dir=fallback_dir,
            )
        except Exception as e:
            print(f"[ERROR] Failed to route {f}: {e}")
            write_log(log_path, f"ERROR routing {f}: {e}")
            continue

        if res.action == "skip":
            skipped += 1
            continue

        routed += 1

        dests = ", ".join(str(d) for d in res.dests) if res.dests else "(none)"
        print(f"- {res.action.upper():<12} {res.src.name}")
        print(f"  reason: {res.reason}")
        print(f"  dest:   {dests}")
        write_log(log_path, f"{res.action} {res.src} -> {dests} | {res.reason}")

    print("")
    print("[RESULT] Done.")
    print(f"- Routed: {routed}")
    print(f"- Skipped (not anchors): {skipped}")

    if dry_run:
        print("")
        print("[NOTE] This was a dry-run. Re-run with --apply to actually move/copy files.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
