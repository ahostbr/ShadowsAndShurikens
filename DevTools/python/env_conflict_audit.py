"""
env_conflict_audit.py
Finds .env files + Python venv folders under a root, highlights key collisions,
and writes a log file (never silent).

Usage:
  python env_conflict_audit.py "E:\\SAS\\ShadowsAndShurikens"
(or run without args from inside the repo)
"""

from __future__ import annotations
import sys
import os
from pathlib import Path
from datetime import datetime

LOG_NAME = "env_conflict_audit.log"

ENV_FILE_PATTERNS = [
    ".env",
    ".env.*",
    "*.env",
]

# folders we don't want to crawl deeply for env files
SKIP_DIR_NAMES = {
    ".git", ".svn", ".hg",
    "node_modules",
    "Binaries", "Intermediate",
    ".vs",
}

def now_ts() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def write_log(lines: list[str], out_path: Path) -> None:
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8", errors="ignore")

def is_probably_venv_dir(p: Path) -> bool:
    # Typical venv markers on Windows
    return (p / "pyvenv.cfg").exists() or (p / "Scripts" / "python.exe").exists()

def should_skip_dir(p: Path) -> bool:
    name = p.name
    if name in SKIP_DIR_NAMES:
        return True
    # skip any venv-ish folder contents
    if name.startswith(".venv") or name.lower() in {"venv", ".venv"}:
        return True
    return False

def parse_env_keys(env_path: Path) -> set[str]:
    keys: set[str] = set()
    try:
        txt = env_path.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return keys

    for raw in txt.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        # handle "export KEY=VALUE"
        if line.lower().startswith("export "):
            line = line[7:].lstrip()
        if "=" not in line:
            continue
        k = line.split("=", 1)[0].strip()
        if not k:
            continue
        # ignore weird keys
        if any(ch.isspace() for ch in k):
            continue
        keys.add(k)
    return keys

def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()
    root = root.resolve()

    lines: list[str] = []
    lines.append(f"[{now_ts()}] env_conflict_audit starting")
    lines.append(f"Root: {root}")
    lines.append("")

    if not root.exists():
        lines.append(f"ERROR: root does not exist: {root}")
        write_log(lines, Path(LOG_NAME))
        print("\n".join(lines))
        return 2

    # 1) Find venvs
    venvs: list[Path] = []
    # only look for likely venv folder names to avoid scanning everything
    candidate_names = {".venv", ".venv_mcp", "venv", ".venv_devtools", ".venv-tools"}
    for p in root.rglob("*"):
        if not p.is_dir():
            continue
        if p.name in candidate_names or p.name.startswith(".venv"):
            if is_probably_venv_dir(p):
                venvs.append(p)

    # Dedup + sort
    venvs = sorted(set(venvs), key=lambda x: str(x).lower())

    lines.append("=== Python Virtual Environments Detected ===")
    if not venvs:
        lines.append("(none found by heuristic)")
    else:
        for v in venvs:
            py = v / "Scripts" / "python.exe"
            lines.append(f"- {v} {'[python.exe OK]' if py.exists() else ''}")
    lines.append("")

    # 2) Find .env files (explicit patterns)
    env_files: list[Path] = []
    # manually walk to support skipping heavy dirs
    stack = [root]
    while stack:
        cur = stack.pop()
        try:
            for child in cur.iterdir():
                if child.is_dir():
                    if should_skip_dir(child):
                        continue
                    stack.append(child)
                else:
                    name = child.name
                    # match patterns without globbing the whole tree repeatedly
                    if name == ".env" or name.startswith(".env.") or name.endswith(".env"):
                        env_files.append(child)
        except PermissionError:
            continue

    env_files = sorted(set(env_files), key=lambda x: str(x).lower())

    lines.append("=== .env Files Detected ===")
    if not env_files:
        lines.append("(none found — if you expected some, your indexer may be filtering them or they're outside this root)")
    else:
        for f in env_files:
            try:
                sz = f.stat().st_size
            except Exception:
                sz = -1
            lines.append(f"- {f} ({sz} bytes)")
    lines.append("")

    # 3) Parse keys + collisions
    key_to_files: dict[str, list[Path]] = {}
    for f in env_files:
        keys = parse_env_keys(f)
        for k in keys:
            key_to_files.setdefault(k, []).append(f)

    collisions = {k: v for k, v in key_to_files.items() if len(v) > 1}
    lines.append("=== Key Collisions (same KEY in multiple .env files) ===")
    if not collisions:
        lines.append("(none detected)")
    else:
        # show the worst offenders first
        for k, files in sorted(collisions.items(), key=lambda kv: (-len(kv[1]), kv[0].lower())):
            lines.append(f"- {k}:")
            for fp in sorted(files, key=lambda x: str(x).lower()):
                lines.append(f"    - {fp}")
    lines.append("")

    # 4) Recommendations (generic, but helpful)
    lines.append("=== Recommendations ===")
    if venvs:
        lines.append("* Pick ONE venv per toolchain and configure VSCode to use it explicitly.")
        lines.append("* Rename or remove extra venvs so auto-detection doesn't flip interpreters.")
    if env_files:
        lines.append("* Avoid a repo-root .env; prefer app-local .env next to the entrypoint.")
        lines.append("* In Python/Node, load dotenv with an explicit path (no upward searching).")
    lines.append("")

    out_path = Path.cwd() / LOG_NAME
    write_log(lines, out_path)
    print("\n".join(lines))
    print(f"\nWrote log: {out_path}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())