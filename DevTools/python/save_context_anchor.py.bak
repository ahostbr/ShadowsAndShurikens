# DevTools/python/save_context_anchor.py
from __future__ import annotations

import argparse
import datetime
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent  # E:\SAS\ShadowsAndShurikens
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "context_anchor.log"

ANCHOR_START = "[CONTEXT_ANCHOR]"
ANCHOR_END = "[/CONTEXT_ANCHOR]"


def log(msg: str) -> None:
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] [ANCHOR] {msg}"
    print(line)
    try:
        with LOG_FILE.open("a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        # Logging must never break the tool.
        pass


def now_tag() -> str:
    return datetime.datetime.now().strftime("%Y%m%d_%H%M%S")


def safe_slug(s: str) -> str:
    s = s.strip().replace(" ", "_")
    s = re.sub(r"[^A-Za-z0-9_\-]+", "", s)
    return s[:64] if s else "ANCHOR"


def parse_anchor_blocks(text: str) -> list[str]:
    blocks: list[str] = []
    start = 0
    while True:
        i = text.find(ANCHOR_START, start)
        if i == -1:
            break
        j = text.find(ANCHOR_END, i)
        if j == -1:
            # Unterminated; treat rest as one block
            blocks.append(text[i:].strip())
            break
        blocks.append(text[i : j + len(ANCHOR_END)].strip())
        start = j + len(ANCHOR_END)
    return blocks


def parse_anchor_kv(anchor_block: str) -> dict[str, str]:
    """
    Parse lines inside the anchor block:
      key: value
    """
    data: dict[str, str] = {}
    for raw in anchor_block.splitlines():
        line = raw.strip()
        if not line or line.startswith("[") or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        data[k.strip().lower()] = v.strip()
    return data


def infer_plugins(anchor_block: str, kv: dict[str, str]) -> list[str]:
    plugins: set[str] = set()

    # Explicit fields (preferred)
    for key in ("plugin", "plugins"):
        val = kv.get(key)
        if val:
            for part in re.split(r"[,\s]+", val):
                p = part.strip()
                if p:
                    plugins.add(p)

    # Lock patterns like Lock_SOTS_UI, Lock_BEP, etc.
    for m in re.finditer(r"\bLock_([A-Za-z0-9_]+)\b", anchor_block):
        plugins.add(m.group(1))

    # Also allow "plugin: X" to be missing but passes mention "plugin: X"
    for m in re.finditer(
        r"\bplugin\s*[:=]\s*([A-Za-z0-9_]+)\b", anchor_block, flags=re.IGNORECASE
    ):
        plugins.add(m.group(1))

    out = [p.strip() for p in plugins if p.strip()]
    out.sort()
    return out


def resolve_anchor_timestamp(kv: dict[str, str]) -> str:
    """
    Best-effort: if anchor has date, we keep it in the filename only if it parses cleanly.
    Otherwise fall back to now_tag().
    """
    raw = (kv.get("date") or "").strip()
    if not raw:
        return now_tag()

    raw2 = raw.replace("T", " ").replace("/", "-")
    for fmt in (
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%d %H:%M",
        "%Y-%m-%d",
    ):
        try:
            dt = datetime.datetime.strptime(raw2, fmt)
            return (
                dt.strftime("%Y%m%d_%H%M%S")
                if "H" in fmt
                else dt.strftime("%Y%m%d_000000")
            )
        except ValueError:
            pass

    return now_tag()


def write_anchor_to_plugin(plugin: str, filename: str, anchor_block: str) -> Path | None:
    plugin_dir = PROJECT_ROOT / "Plugins" / plugin
    if not plugin_dir.exists():
        log(f"[WARN] Plugin folder not found: {plugin_dir}")
        return None

    dest_dir = plugin_dir / "Docs" / "Anchor"
    dest_dir.mkdir(parents=True, exist_ok=True)

    dest = dest_dir / filename
    if dest.exists():
        # Avoid overwrite: add numeric suffix
        stem = dest.stem
        ext = dest.suffix or ".md"
        for i in range(1, 1000):
            cand = dest_dir / f"{stem}_{i:02d}{ext}"
            if not cand.exists():
                dest = cand
                break

    dest.write_text(anchor_block.strip() + "\n", encoding="utf-8")
    return dest


def also_write_central_copy(filename: str, anchor_block: str) -> Path:
    central = ROOT / "Saved" / "ContextAnchors"
    central.mkdir(parents=True, exist_ok=True)
    dest = central / filename
    if dest.exists():
        stem = dest.stem
        ext = dest.suffix or ".md"
        for i in range(1, 1000):
            cand = central / f"{stem}_{i:02d}{ext}"
            if not cand.exists():
                dest = cand
                break
    dest.write_text(anchor_block.strip() + "\n", encoding="utf-8")
    return dest


def process_text(text: str, *, default_slug: str | None = None) -> int:
    blocks = parse_anchor_blocks(text)
    if not blocks:
        log("No [CONTEXT_ANCHOR] blocks found in input.")
        return 2

    wrote = 0
    for block in blocks:
        kv = parse_anchor_kv(block)
        plugins = infer_plugins(block, kv)

        ts = resolve_anchor_timestamp(kv)
        slug = safe_slug(
            (kv.get("slug") or default_slug or (plugins[0] if len(plugins) == 1 else "MULTI"))
        )
        filename = f"CONTEXT_ANCHOR_{ts}_{slug}.md"

        central_path = also_write_central_copy(filename, block)
        log(f"Wrote central copy: {central_path}")

        if not plugins:
            log("[WARN] No plugin could be inferred. Central copy written only.")
            continue

        for plugin in plugins:
            out_path = write_anchor_to_plugin(plugin, filename, block)
            if out_path:
                wrote += 1
                log(f"Wrote plugin copy: {out_path}")

    log(f"Done. Plugin copies written: {wrote}")
    return 0


def scan_inbox(*, move_processed: bool) -> int:
    inbox = ROOT / "chatgpt_inbox"
    if not inbox.exists():
        log(f"[ERROR] chatgpt_inbox not found: {inbox}")
        return 2

    candidates: list[Path] = []
    for p in inbox.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in {".txt", ".md", ".log"}:
            continue
        try:
            txt = p.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue
        if ANCHOR_START in txt:
            candidates.append(p)

    if not candidates:
        log("No inbox files containing [CONTEXT_ANCHOR] were found.")
        return 0

    candidates.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    log(f"Found {len(candidates)} inbox candidate(s).")

    processed_dir = ROOT / "Saved" / "ContextAnchors" / "inbox_processed"
    processed_dir.mkdir(parents=True, exist_ok=True)

    ok = 0
    for p in candidates:
        log(f"Processing inbox file: {p}")
        txt = p.read_text(encoding="utf-8", errors="replace")
        rc = process_text(txt, default_slug=p.stem)
        if rc == 0:
            ok += 1
            if move_processed:
                try:
                    rel = p.relative_to(inbox)
                except Exception:
                    rel = Path(p.name)

                dest = processed_dir / f"{now_tag()}__{safe_slug(str(rel))}{p.suffix}"
                dest.parent.mkdir(parents=True, exist_ok=True)
                try:
                    dest.write_text(txt, encoding="utf-8")
                    p.unlink(missing_ok=True)
                    log(f"Moved processed file -> {dest}")
                except Exception as e:
                    log(f"[WARN] Could not move/delete processed inbox file: {e}")
        else:
            log(f"[WARN] Processing failed for {p} (rc={rc}). Leaving file in place.")

    log(f"Inbox scan complete. Processed OK: {ok}/{len(candidates)}")
    return 0


def interactive() -> int:
    print("")
    print("=== Save Context Anchor ===")
    print("1) Paste a [CONTEXT_ANCHOR] block now")
    print("2) Process a file path containing [CONTEXT_ANCHOR]")
    print("3) Scan chatgpt_inbox for [CONTEXT_ANCHOR] blocks")
    print("")
    choice = input("Choice (1/2/3)> ").strip()

    if choice == "1":
        print("")
        print("Paste the full block (end with a single line containing only: EOF)")
        lines: list[str] = []
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            if line.strip() == "EOF":
                break
            lines.append(line)
        return process_text("".join(lines), default_slug="PASTE")

    if choice == "2":
        path = input("File path> ").strip().strip('"')
        p = Path(path)
        if not p.is_file():
            log(f"[ERROR] File not found: {p}")
            return 2
        return process_text(p.read_text(encoding="utf-8", errors="replace"), default_slug=p.stem)

    if choice == "3":
        return scan_inbox(move_processed=True)

    log("[WARN] Unknown choice.")
    return 2


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", help="Path to a prompt/text file that contains one or more [CONTEXT_ANCHOR] blocks.")
    ap.add_argument("--scan-inbox", action="store_true", help="Scan DevTools/python/chatgpt_inbox for context anchors.")
    ap.add_argument("--move-processed", action="store_true", help="When scanning inbox, move processed files to Saved/ContextAnchors/inbox_processed and delete originals.")
    ap.add_argument("--slug", help="Optional default slug if the anchor block does not provide one.")
    args = ap.parse_args(argv)

    if args.scan_inbox:
        return scan_inbox(move_processed=args.move_processed)

    if args.source:
        p = Path(args.source)
        if not p.is_file():
            log(f"[ERROR] --source file not found: {p}")
            return 2
        return process_text(p.read_text(encoding="utf-8", errors="replace"), default_slug=args.slug or p.stem)

    return interactive()


if __name__ == "__main__":
    raise SystemExit(main())
