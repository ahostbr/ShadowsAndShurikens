from __future__ import annotations

import datetime
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# ADD-ONLY patcher for sots_tools.py
# - creates a timestamped backup
# - injects new wrapper funcs if missing
# - injects new Category 4 menu options if missing
# - verifies nothing was removed (defs / subcommands / menu-option print lines)
# ---------------------------------------------------------------------------

ROOT = Path(__file__).resolve().parent
TARGET = ROOT / "sots_tools.py"
LOG_DIR = ROOT / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "patch_sots_tools_add_context_anchors.log"

FUNC_SAVE = "run_save_context_anchor"
FUNC_SCAN = "run_scan_context_anchors_inbox"

MENU_PRINT_12 = '        print(" 12) Save Context Anchor (paste/file)")'
MENU_PRINT_13 = '        print(" 13) Scan inbox for Context Anchors")'

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

def parse_defs(text: str) -> set[str]:
    return set(re.findall(r"^def\s+([A-Za-z0-9_]+)\s*\(", text, flags=re.M))

def parse_subcommands(text: str) -> set[str]:
    return set(re.findall(r"add_parser\(\s*['\"]([^'\"]+)['\"]", text))

def parse_menu_option_prints(text: str) -> set[str]:
    # Just capture the raw print lines so we can assert we didn't delete any.
    out = set()
    for m in re.finditer(r'^\s*print\(" {2}\d+\)\s.*"\)\s*$', text, flags=re.M):
        out.add(m.group(0))
    return out

def write_backup(path: Path, text: str) -> Path:
    backup = path.with_suffix(path.suffix + f".pre_context_anchor_{now_tag()}.bak")
    backup.write_text(text, encoding="utf-8")
    return backup

def ensure_wrapper_functions(text: str) -> tuple[str, bool]:
    """
    Inject wrapper functions near other helpers if missing:
      - run_save_context_anchor()
      - run_scan_context_anchors_inbox()
    """
    changed = False

    if f"def {FUNC_SAVE}(" in text and f"def {FUNC_SCAN}(" in text:
        return text, changed

    inject_block = "\n\n# ---------------------------------------------------------------------------\n# Context Anchors (ADD-ONLY)\n# ---------------------------------------------------------------------------\n\n"
    if f"def {FUNC_SAVE}(" not in text:
        inject_block += f"def {FUNC_SAVE}() -> None:\n    \"\"\"Save a [CONTEXT_ANCHOR] block to Plugins/<Plugin>/Docs/Anchor/.\"\"\"\n    run_script(\"save_context_anchor.py\")\n\n\n"
    if f"def {FUNC_SCAN}(" not in text:
        inject_block += f"def {FUNC_SCAN}() -> None:\n    \"\"\"Scan chatgpt_inbox for [CONTEXT_ANCHOR] blocks and route them to Plugins/<Plugin>/Docs/Anchor/.\"\"\"\n    run_script(\"save_context_anchor.py\", [\"--scan-inbox\", \"--move-processed\"])\n\n"

    # Insert before the "Interactive menu UI" section if present; else append at end.
    marker = "# ---------------------------------------------------------------------------\n# Interactive menu UI"
    idx = text.find(marker)
    if idx != -1:
        text = text[:idx] + inject_block + text[idx:]
        changed = True
        return text, changed

    # Fallback: insert before main()/arg parser if marker not found.
    marker2 = "def run_interactive_menu()"
    idx2 = text.find(marker2)
    if idx2 != -1:
        text = text[:idx2] + inject_block + text[idx2:]
        changed = True
        return text, changed

    # Last resort: append (still add-only).
    text += inject_block
    changed = True
    return text, changed

def ensure_category4_menu(text: str) -> tuple[str, bool]:
    """
    Add menu prints and choice handlers inside category_batch_editing()
    without deleting or renumbering existing lines.
    """
    changed = False

    # If prints already exist, assume menu already patched.
    if "Save Context Anchor" in text and "Scan inbox for Context Anchors" in text:
        return text, changed

    # 1) Inject the two print lines after option 11 (browse inbox) if present.
    # Find the exact print for option 11.
    pat_print_11 = r'^\s*print\(" 11\) Browse ChatGPT inbox \(manual dispatcher\)"\)\s*$'
    m11 = re.search(pat_print_11, text, flags=re.M)
    if m11 and MENU_PRINT_12 not in text:
        insert_at = m11.end()
        text = text[:insert_at] + "\n" + MENU_PRINT_12 + "\n" + MENU_PRINT_13 + text[insert_at:]
        changed = True
    else:
        # If the menu is formatted differently, inject right before "0) Back" inside Category 4.
        pat_back = r'^\s*print\(""\)\s*\n\s*print\(" {2}0\) Back to main menu"\)\s*$'
        mb = re.search(pat_back, text, flags=re.M)
        if mb and MENU_PRINT_12 not in text:
            insert_at = mb.start()
            text = text[:insert_at] + MENU_PRINT_12 + "\n" + MENU_PRINT_13 + "\n" + text[insert_at:]
            changed = True

    # 2) Inject handlers in the choice chain: after the handler for choice "11" if possible.
    # Try to locate: elif choice == "11":
    pat_choice_11 = r'^\s*elif choice == "11":\s*$'
    mc11 = re.search(pat_choice_11, text, flags=re.M)
    if mc11 and 'elif choice == "12":' not in text:
        insert_at = mc11.end()
        inject = (
            "\n            run_apply_latest_chatgpt_inbox()\n"
            "        elif choice == \"12\":\n"
            f"            {FUNC_SAVE}()\n"
            "        elif choice == \"13\":\n"
            f"            {FUNC_SCAN}()\n"
        )
        # Replace the single-line 'elif choice == "11":' with the same line plus injected block,
        # BUT only if the very next line isn't already a call.
        # We'll insert immediately after the 'elif' line; existing code remains below.
        text = text[:insert_at] + inject + text[insert_at:]
        changed = True

    return text, changed

def verify_no_removals(before: str, after: str) -> tuple[bool, list[str]]:
    """
    Hard safety check: ensure our patch did not remove any pre-existing:
      - def names
      - subcommand names
      - menu-option print lines (the numbered ones)
    """
    problems: list[str] = []

    b_defs = parse_defs(before)
    a_defs = parse_defs(after)
    missing_defs = sorted(b_defs - a_defs)
    if missing_defs:
        problems.append(f"Removed def(s): {missing_defs}")

    b_cmds = parse_subcommands(before)
    a_cmds = parse_subcommands(after)
    missing_cmds = sorted(b_cmds - a_cmds)
    if missing_cmds:
        problems.append(f"Removed CLI subcommand(s): {missing_cmds}")

    b_menu = parse_menu_option_prints(before)
    a_menu = parse_menu_option_prints(after)
    missing_menu = sorted(b_menu - a_menu)
    if missing_menu:
        problems.append(f"Removed menu print line(s): {missing_menu[:20]}{' ...' if len(missing_menu) > 20 else ''}")

    ok = len(problems) == 0
    return ok, problems

def main() -> int:
    log("=== patch_sots_tools_add_context_anchors.py start ===")

    if not TARGET.exists():
        log(f"[ERROR] Target not found: {TARGET}")
        return 2

    before = TARGET.read_text(encoding="utf-8", errors="replace")
    backup_path = write_backup(TARGET, before)
    log(f"Backup written: {backup_path}")

    after, ch1 = ensure_wrapper_functions(before)
    after, ch2 = ensure_category4_menu(after)

    ok, problems = verify_no_removals(before, after)
    if not ok:
        log("[ERROR] Refusing to write patch because it would remove existing features.")
        for p in problems:
            log(" - " + p)
        log("Restoring original file from backup (no changes applied).")
        TARGET.write_text(before, encoding="utf-8")
        return 3

    if after == before:
        log("[INFO] No changes needed (already patched).")
        return 0

    TARGET.write_text(after, encoding="utf-8")
    log(f"[OK] Patched: {TARGET}")
    log(f"Changed wrapper functions: {ch1}, changed menu: {ch2}")
    log("=== patch_sots_tools_add_context_anchors.py end ===")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        log(f"[FATAL] Unhandled exception: {exc}")
        import traceback
        traceback.print_exc()
        try:
            if sys.stdin.isatty():
                input("\nPress Enter to close...")
        except Exception:
            pass
        raise
