from __future__ import annotations

import json
import sys
from datetime import datetime
from pathlib import Path


LOG_PREFIX = "[PACK]"


def get_tools_root() -> Path:
    """
    Returns the DevTools/python directory (where this script lives).
    """
    return Path(__file__).resolve().parent


def get_project_root(config: dict | None = None) -> Path:
    """
    Determine the project root.

    Priority:
      1) config["project_root"] if present
      2) DevTools/.. (assuming DevTools/python/)
    """
    tools_root = get_tools_root()
    if config and "project_root" in config:
        return Path(config["project_root"]).resolve()
    # Fallback: DevTools/python -> DevTools -> project root
    return tools_root.parent.parent.resolve()


def load_config() -> dict:
    """
    Load sots_tools_config.json if present.
    Used mainly to discover project_root.
    """
    cfg_path = get_tools_root() / "sots_tools_config.json"
    if not cfg_path.exists():
        print(f"{LOG_PREFIX} No config file found at {cfg_path}, using defaults.")
        return {}

    try:
        data = json.loads(cfg_path.read_text(encoding="utf-8"))
        print(f"{LOG_PREFIX} Loaded config from {cfg_path}")
        return data
    except Exception as e:
        print(f"{LOG_PREFIX} Failed to parse config at {cfg_path}: {e}")
        return {}


def resolve_pack_path(raw: str, project_root: Path) -> Path:
    """
    Resolve the JSON pack path from user or CLI input.

    - Absolute path: used as-is.
    - Relative path: treated as relative to project_root.
    """
    p = Path(raw)
    if p.is_absolute():
        return p
    return (project_root / p).resolve()


def apply_replace_in_file(op: dict, project_root: Path) -> None:
    rel = op.get("file_path") or op.get("file")
    search = op.get("search")
    replace = op.get("replace")

    if not rel or search is None or replace is None:
        print(f"{LOG_PREFIX} [WARN] replace_in_file missing required fields: {op.get('id')}")
        return

    target = (project_root / rel).resolve()
    if not target.exists():
        print(f"{LOG_PREFIX} [WARN] File not found for replace_in_file: {target}")
        return

    text = target.read_text(encoding="utf-8")
    if search not in text:
        print(f"{LOG_PREFIX} [WARN] Pattern not found in {rel} for op {op.get('id')}")
    else:
        text = text.replace(search, replace)
        target.write_text(text, encoding="utf-8")
        print(f"{LOG_PREFIX} [OK] replace_in_file applied to {rel} (op {op.get('id')})")


def apply_insert_after(op: dict, project_root: Path) -> None:
    rel = op.get("file_path") or op.get("file")
    anchor = op.get("anchor")
    insert = op.get("insert_text")

    if not rel or anchor is None or insert is None:
        print(f"{LOG_PREFIX} [WARN] insert_after missing required fields: {op.get('id')}")
        return

    target = (project_root / rel).resolve()
    if not target.exists():
        print(f"{LOG_PREFIX} [WARN] File not found for insert_after: {target}")
        return

    text = target.read_text(encoding="utf-8")
    idx = text.find(anchor)
    if idx == -1:
        print(f"{LOG_PREFIX} [WARN] Anchor not found in {rel} for op {op.get('id')}")
        return

    insert_pos = idx + len(anchor)
    new_text = text[:insert_pos] + insert + text[insert_pos:]
    target.write_text(new_text, encoding="utf-8")
    print(f"{LOG_PREFIX} [OK] insert_after applied to {rel} (op {op.get('id')})")


def apply_insert_at_top(op: dict, project_root: Path) -> None:
    rel = op.get("file_path") or op.get("file")
    insert = op.get("text") or op.get("insert_text")

    if not rel or insert is None:
        print(f"{LOG_PREFIX} [WARN] insert_at_top missing required fields: {op.get('id')}")
        return

    target = (project_root / rel).resolve()
    if not target.exists():
        print(f"{LOG_PREFIX} [WARN] File not found for insert_at_top: {target}")
        return

    text = target.read_text(encoding="utf-8")
    new_text = insert + text
    target.write_text(new_text, encoding="utf-8")
    print(f"{LOG_PREFIX} [OK] insert_at_top applied to {rel} (op {op.get('id')})")


def apply_operation(op: dict, project_root: Path) -> None:
    op_type = op.get("type")
    if not op_type:
        print(f"{LOG_PREFIX} [WARN] Operation missing 'type': {op}")
        return

    if op_type == "replace_in_file":
        apply_replace_in_file(op, project_root)
    elif op_type == "insert_after":
        apply_insert_after(op, project_root)
    elif op_type == "insert_at_top":
        apply_insert_at_top(op, project_root)
    else:
        print(f"{LOG_PREFIX} [WARN] Unsupported op type '{op_type}' (id {op.get('id')})")


def main() -> None:
    tools_root = get_tools_root()
    config = load_config()
    project_root = get_project_root(config)

    print(f"{LOG_PREFIX} Tools root    : {tools_root}")
    print(f"{LOG_PREFIX} Project root  : {project_root}")

    # 1) Determine pack path from CLI or prompt.
    pack_arg: str | None = None
    if len(sys.argv) > 1:
        pack_arg = sys.argv[1]

    if not pack_arg:
        print("")
        print("=== Apply JSON DevTools Pack ===")
        print("Enter the path to a JSON pack file.")
        print(" - Absolute path or")
        print(" - Path relative to project root")
        pack_arg = input("Pack path> ").strip()

    if not pack_arg:
        print(f"{LOG_PREFIX} No pack path provided. Aborting.")
        return

    pack_path = resolve_pack_path(pack_arg, project_root)

    if not pack_path.exists():
        print(f"{LOG_PREFIX} [ERROR] Pack file not found: {pack_path}")
        return

    # 2) Load pack JSON.
    try:
        pack_data = json.loads(pack_path.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"{LOG_PREFIX} [ERROR] Failed to parse pack JSON: {e}")
        return

    ops = pack_data.get("operations", [])
    print("")
    print(f"{LOG_PREFIX} Pack name     : {pack_data.get('pack_name')}")
    print(f"{LOG_PREFIX} Version       : {pack_data.get('pack_version')}")
    print(f"{LOG_PREFIX} Description   : {pack_data.get('description')}")
    print(f"{LOG_PREFIX} Operations    : {len(ops)}")
    print(f"{LOG_PREFIX} Pack file     : {pack_path}")
    print("")

    if not ops:
        print(f"{LOG_PREFIX} [WARN] Pack has no operations. Nothing to do.")
        return

    confirm = input("Apply all operations? [y/N] ").strip().lower()
    if confirm not in {"y", "yes"}:
        print(f"{LOG_PREFIX} Aborted by user.")
        return

    # 3) Apply operations.
    log_lines: list[str] = []
    start_ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log_lines.append(f"{LOG_PREFIX} Run started : {start_ts}")
    log_lines.append(f"{LOG_PREFIX} Pack        : {pack_path}")
    log_lines.append(f"{LOG_PREFIX} Project     : {project_root}")
    log_lines.append("")

    for op in ops:
        op_id = op.get("id", "<no-id>")
        desc = op.get("description", "")
        log_lines.append(f"{LOG_PREFIX} Applying op {op_id}: {desc}")
        print(f"{LOG_PREFIX} Applying op {op_id}: {desc}")
        apply_operation(op, project_root)

    end_ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    log_lines.append("")
    log_lines.append(f"{LOG_PREFIX} Run finished: {end_ts}")

    # 4) Write a simple log file.
    log_path = tools_root / "devtools_pack_last_run.log"
    try:
        log_path.write_text("\n".join(log_lines), encoding="utf-8")
        print(f"{LOG_PREFIX} Log written to {log_path}")
    except Exception as e:
        print(f"{LOG_PREFIX} [WARN] Failed to write log file: {e}")

    print(f"{LOG_PREFIX} Done.")


if __name__ == "__main__":
    main()
