from __future__ import annotations

import argparse
import datetime
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent  # .../ShadowsAndShurikens
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "context_anchor.log"

ANCHOR_START = "[CONTEXT_ANCHOR]"
ANCHOR_END = "[/CONTEXT_ANCHOR]"

# Keep a central copy as well (handy for auditing)
CENTRAL_DIR = ROOT / "Saved" / "ContextAnchors"
CENTRAL_DIR.mkdir(parents=True, exist_ok=True)

# Shorthand aliases -> actual plugin folder names
PLUGIN_ALIASES: dict[str, str] = {
    "KEM": "SOTS_KillExecutionManager",
    "SOTS_KEM": "SOTS_KillExecutionManager",
    "GSM": "SOTS_GlobalStealthManager",
    "SOTS_GSM": "SOTS_GlobalStealthManager",
    "AIP": "SOTS_AIPerception",
    "SOTS_AIP": "SOTS_AIPerception",
    "MD": "SOTS_MissionDirector",
    "SOTS_MD": "SOTS_MissionDirector",
    "INV": "SOTS_INV",
    "UI": "SOTS_UI",
    "MMSS": "SOTS_MMSS",
    "STATS": "SOTS_Stats",
    "STEAM": "SOTS_Steam",
    "PROFILESHARED": "SOTS_ProfileShared",
    "TAGMANAGER": "SOTS_TagManager",
    "FX": "SOTS_FX_Plugin",
    "PARKOUR": "SOTS_Parkour",
    "EDUTIL": "SOTS_EdUtil",
    "DEBUG": "SOTS_Debug",
    "BLUEPRINTGEN": "SOTS_BlueprintGen",
    "BEP": "BEP",
    "OMNITRACE": "OmniTrace",
    "LIGHTPROBE": "LightProbePlugin",
    "BLUEPRINTCOMMENTLINKS": "BlueprintCommentLinks",
}

def log(msg: str) -> None:
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    line = f"[{ts}] {msg}"
    print(line)
    try:
        LOG_FILE.write_text(LOG_FILE.read_text(encoding="utf-8", errors="replace") + line + "\n", encoding="utf-8")
    except Exception:
        try:
            LOG_FILE.write_text(line + "\n", encoding="utf-8")
        except Exception:
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
        j = text.find(ANCHOR_END, i + len(ANCHOR_START))
        if j == -1:
            blocks.append(text[i:].strip())
            break
        blocks.append(text[i : j + len(ANCHOR_END)].strip())
        start = j + len(ANCHOR_END)
    return blocks

def parse_kv(block: str) -> dict[str, str]:
    kv: dict[str, str] = {}
    for line in block.splitlines():
        m = re.match(r"^\s*([A-Za-z0-9_\-]+)\s*:\s*(.*?)\s*$", line)
        if not m:
            continue
        k = m.group(1).strip().lower()
        v = m.group(2).strip()
        if k and v:
            kv[k] = v
    return kv

def resolve_anchor_timestamp(kv: dict[str, str]) -> str:
    for key in ("created", "timestamp", "ts", "dt", "date"):
        val = kv.get(key)
        if not val:
            continue
        m = re.search(r"\b(\d{8})[_-](\d{6})\b", val)
        if m:
            return f"{m.group(1)}_{m.group(2)}"
    return now_tag()

def list_plugin_folders() -> list[str]:
    plugins_root = PROJECT_ROOT / "Plugins"
    if not plugins_root.exists():
        return []
    out: list[str] = []
    for p in plugins_root.iterdir():
        if p.is_dir() and any(p.glob("*.uplugin")):
            out.append(p.name)
    return sorted(out)

def resolve_plugin_folder(plugin_id: str) -> tuple[str | None, str]:
    raw = plugin_id.strip()
    if not raw:
        return None, "empty"
    if raw.lower().startswith("lock_"):
        raw = raw[5:]

    folders = list_plugin_folders()
    if not folders:
        return None, "no plugin folders found"

    # direct
    direct = PROJECT_ROOT / "Plugins" / raw
    if direct.exists():
        return raw, "direct"

    # case-insensitive
    for f in folders:
        if f.lower() == raw.lower():
            return f, "case-insensitive"

    # alias
    mapped = PLUGIN_ALIASES.get(raw.upper())
    if mapped:
        for f in folders:
            if f.lower() == mapped.lower():
                return f, f"alias({raw}->{f})"

    # heuristic substring
    raw_n = re.sub(r"[^a-z0-9]+", "", raw.lower())
    hits = []
    for f in folders:
        f_n = re.sub(r"[^a-z0-9]+", "", f.lower())
        if raw_n and (raw_n in f_n or f_n in raw_n):
            hits.append(f)
    hits = sorted(set(hits))
    if len(hits) == 1:
        return hits[0], f"heuristic({raw}->{hits[0]})"
    if len(hits) > 1:
        return None, f"ambiguous({raw}->{hits})"

    return None, f"unresolved({plugin_id})"

def infer_plugins(anchor_block: str, kv: dict[str, str]) -> list[str]:
    plugins: set[str] = set()

    for key in ("plugin", "plugins"):
        val = kv.get(key)
        if val:
            for part in re.split(r"[,\s]+", val):
                p = part.strip()
                if p:
                    plugins.add(p)

    for m in re.finditer(r"\bLock_([A-Za-z0-9_]+)\b", anchor_block):
        plugins.add(m.group(1))

    return sorted(plugins)

def write_central_copy(filename: str, anchor_block: str) -> Path:
    dest = CENTRAL_DIR / filename
    if dest.exists():
        dest = CENTRAL_DIR / f"{dest.stem}_{now_tag()}{dest.suffix}"
    dest.write_text(anchor_block.strip() + "\n", encoding="utf-8")
    return dest

def write_to_plugin(plugin: str, filename: str, anchor_block: str) -> Path | None:
    folder, reason = resolve_plugin_folder(plugin)
    if not folder:
        log(f"[WARN] Plugin resolve failed for '{plugin}' ({reason}).")
        return None

    plugin_dir = PROJECT_ROOT / "Plugins" / folder
    dest_dir = plugin_dir / "Docs" / "Anchor"
    dest_dir.mkdir(parents=True, exist_ok=True)

    dest = dest_dir / filename
    if dest.exists():
        dest = dest_dir / f"{dest.stem}_{now_tag()}{dest.suffix}"
    dest.write_text(anchor_block.strip() + "\n", encoding="utf-8")
    return dest

def process_text(text: str, default_slug: str | None = None) -> int:
    blocks = parse_anchor_blocks(text)
    if not blocks:
        log("No [CONTEXT_ANCHOR] blocks found.")
        return 2

    rc = 0
    for block in blocks:
        kv = parse_kv(block)
        plugins = infer_plugins(block, kv)
        ts = resolve_anchor_timestamp(kv)
        slug = safe_slug(kv.get("slug") or default_slug or (plugins[0] if len(plugins) == 1 else "MULTI"))
        filename = f"CONTEXT_ANCHOR_{ts}_{slug}.md"

        cpath = write_central_copy(filename, block)
        log(f"Wrote central copy: {cpath}")

        if not plugins:
            log("[WARN] No plugin inferred. Central copy only.")
            continue

        for plugin in plugins:
            out = write_to_plugin(plugin, filename, block)
            if out:
                log(f"Wrote plugin copy: {out}")
            else:
                rc = 1
    return rc

def scan_inbox(move_processed: bool) -> int:
    inbox = ROOT / "chatgpt_inbox"
    if not inbox.exists():
        log(f"[ERROR] chatgpt_inbox not found: {inbox}")
        return 2

    processed_dir = CENTRAL_DIR / "inbox_processed"
    processed_dir.mkdir(parents=True, exist_ok=True)

    files = []
    for p in inbox.rglob("*"):
        if p.is_file() and p.suffix.lower() in {".txt", ".md", ".log"}:
            try:
                txt = p.read_text(encoding="utf-8", errors="replace")
            except Exception:
                continue
            if ANCHOR_START in txt:
                files.append(p)

    if not files:
        log("No inbox files contain [CONTEXT_ANCHOR].")
        return 0

    log(f"Found {len(files)} inbox file(s) with anchors.")
    ok = 0
    for p in sorted(files, key=lambda x: x.stat().st_mtime, reverse=True):
        log(f"Processing inbox file: {p}")
        txt = p.read_text(encoding="utf-8", errors="replace")
        rc = process_text(txt)
        if rc == 0:
            ok += 1
            if move_processed:
                dest = processed_dir / p.name
                if dest.exists():
                    dest = processed_dir / f"{p.stem}_{now_tag()}{p.suffix}"
                try:
                    p.replace(dest)
                    log(f"Moved processed inbox file -> {dest}")
                except Exception as exc:
                    log(f"[WARN] Could not move processed file: {p} ({exc})")
        else:
            log(f"[WARN] Failed processing: {p} (rc={rc})")

    log(f"Inbox scan complete. OK: {ok}/{len(files)}")
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
        print("Paste your [CONTEXT_ANCHOR] block(s). End input with a single line containing only: EOF")
        buf: list[str] = []
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            if line.strip() == "EOF":
                break
            buf.append(line)
        return process_text("".join(buf))

    if choice == "2":
        path_str = input("File path> ").strip().strip('"')
        p = Path(path_str)
        if not p.exists():
            log(f"[ERROR] File not found: {p}")
            return 2
        return process_text(p.read_text(encoding="utf-8", errors="replace"))

    if choice == "3":
        return scan_inbox(move_processed=True)

    print("[INFO] Cancelled.")
    return 0

def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Save [CONTEXT_ANCHOR] blocks into Plugins/<Plugin>/Docs/Anchor/")
    ap.add_argument("--file", type=str, default=None, help="File containing one or more [CONTEXT_ANCHOR] blocks")
    ap.add_argument("--slug", type=str, default=None, help="Default slug if anchor does not specify one")
    ap.add_argument("--scan-inbox", action="store_true", help="Scan chatgpt_inbox for anchors")
    ap.add_argument("--move-processed", action="store_true", help="Move processed inbox files to Saved/ContextAnchors/inbox_processed")
    ap.add_argument("--interactive", action="store_true", help="Interactive mode")
    args = ap.parse_args(argv)

    log("=== save_context_anchor.py start ===")

    if args.interactive or (not args.file and not args.scan_inbox):
        rc = interactive()
        log(f"=== save_context_anchor.py end (rc={rc}) ===")
        return rc

    if args.scan_inbox:
        rc = scan_inbox(move_processed=args.move_processed)
        log(f"=== save_context_anchor.py end (rc={rc}) ===")
        return rc

    if args.file:
        p = Path(args.file).expanduser()
        if not p.exists():
            log(f"[ERROR] File not found: {p}")
            return 2
        txt = p.read_text(encoding="utf-8", errors="replace")
        rc = process_text(txt, default_slug=args.slug)
        log(f"=== save_context_anchor.py end (rc={rc}) ===")
        return rc

    log("Nothing to do.")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception:
        import traceback
        log("[FATAL] Unhandled exception in save_context_anchor.py")
        traceback.print_exc()
        try:
            if sys.stdin.isatty():
                input("\nPress Enter to close...")
        except Exception:
            pass
        raise
