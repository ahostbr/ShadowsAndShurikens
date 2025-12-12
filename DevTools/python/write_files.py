# DevTools/python/write_files.py
from __future__ import annotations

import argparse
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent  # E:\SAS\ShadowsAndShurikens
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)


def now_tag() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def parse_file_blocks(body: str) -> list[tuple[str, str]]:
    """
    Parse blocks of the form:

        === FILE: relative/path/to/File.cpp ===
        ...content...
        === END FILE ===

    '=== END FILE ===' is optional; a new FILE header or EOF closes the current block.
    """
    files: list[tuple[str, str]] = []

    current_path: str | None = None
    current_lines: list[str] = []

    def flush():
        nonlocal current_path, current_lines
        if current_path is not None:
            content = "\n".join(current_lines)
            # Ensure file ends with a newline (nice for compilers/diffs)
            if not content.endswith("\n"):
                content += "\n"
            files.append((current_path, content))
        current_path = None
        current_lines = []

    for raw_line in body.splitlines():
        line = raw_line.rstrip("\n")

        stripped = line.strip()
        if stripped.startswith("=== FILE:") and stripped.endswith("==="):
            # New file block
            flush()
            inner = stripped[len("=== FILE:") : -3].strip()
            current_path = inner
            current_lines = []
        elif stripped == "=== END FILE ===":
            flush()
        else:
            if current_path is not None:
                current_lines.append(line)

    # Flush any trailing file without explicit END
    flush()

    return files


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description="SOTS DevTools: write one or more files from a ChatGPT prompt."
    )
    ap.add_argument(
        "--source",
        required=True,
        help="Path to the full prompt .txt file (as saved in chatgpt_inbox).",
    )
    args = ap.parse_args(argv)

    prompt_path = Path(args.source).resolve()
    if not prompt_path.is_file():
        print(f"[WRITE_FILES] ERROR: source not found: {prompt_path}")
        return 1

    text = prompt_path.read_text(encoding="utf-8", errors="replace")

    start_tag = "[SOTS_DEVTOOLS]"
    end_tag = "[/SOTS_DEVTOOLS]"

    start_idx = text.find(start_tag)
    end_idx = text.find(end_tag)

    if start_idx == -1 or end_idx == -1 or end_idx < start_idx:
        print("[WRITE_FILES] ERROR: No [SOTS_DEVTOOLS] header block found.")
        return 1

    body = text[end_idx + len(end_tag) :]

    files = parse_file_blocks(body)
    ts = now_tag()
    log_path = LOG_DIR / f"write_files_{ts}.log"

    if not files:
        with log_path.open("w", encoding="utf-8") as log:
            print("[WRITE_FILES] No file blocks found in body.", file=log)
        print(f"[WRITE_FILES] No file blocks found. Log: {log_path}")
        return 0

    created = 0
    updated = 0

    with log_path.open("w", encoding="utf-8") as log:
        print(f"[WRITE_FILES] source={prompt_path}", file=log)
        print(f"[WRITE_FILES] PROJECT_ROOT={PROJECT_ROOT}", file=log)

        for rel_path, content in files:
            rel_path = rel_path.replace("\\", "/")
            dest = (PROJECT_ROOT / rel_path).resolve()

            # Safety: ensure we're still under PROJECT_ROOT
            try:
                dest.relative_to(PROJECT_ROOT)
            except ValueError:
                print(
                    f"[SKIP] {dest} is outside PROJECT_ROOT; path={rel_path!r}",
                    file=log,
                )
                continue

            dest.parent.mkdir(parents=True, exist_ok=True)

            mode = "updated" if dest.exists() else "created"
            dest.write_text(content, encoding="utf-8")

            if mode == "created":
                created += 1
            else:
                updated += 1

            print(f"[WRITE_FILES] {mode.upper()}: {dest}", file=log)

        print(
            f"\n[WRITE_FILES] Summary: created={created} updated={updated}",
            file=log,
        )

    print(
        f"[WRITE_FILES] Done. created={created} updated={updated}. Log: {log_path}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
