from __future__ import annotations
import argparse
import json
from pathlib import Path
from typing import List, Dict, Any

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def load_rules(cfg_path: Path) -> Dict[str, Any]:
    if not cfg_path.exists():
        return {"rules": []}
    try:
        with cfg_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"[WARN] Failed to read architecture lint config: {e}")
        return {"rules": []}


def _normalize_path_part(part: str) -> str:
    return part.replace("\\", "/").strip("/")


def _is_allowed_path(p: Path, allow_paths: List[str], project_root: Path) -> bool:
    if not allow_paths:
        return False
    try:
        rel = str(p.relative_to(project_root)).replace("\\", "/")
    except ValueError:
        rel = str(p)
    rel = rel.lstrip("/")
    for allow in allow_paths:
        allow_norm = _normalize_path_part(allow)
        if not allow_norm:
            continue
        if rel.startswith(allow_norm):
            return True
    return False


def scan_rule(rule: Dict[str, Any], project_root: Path) -> (str, List[str]):
    name = rule.get("name", "UnnamedRule")
    include_paths = rule.get("include_paths", [])
    forbidden = rule.get("forbid", [])
    allow_paths = rule.get("allow_paths", [])

    violations: List[str] = []

    if not include_paths or not forbidden:
        return name, violations

    for rel in include_paths:
        base = project_root / rel
        if not base.exists():
            continue
        for p in base.rglob("*"):
            if not p.is_file():
                continue
            if p.suffix.lower() not in (".h", ".cpp", ".cs"):
                continue
            if _is_allowed_path(p, allow_paths, project_root):
                continue
            try:
                text = p.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue
            for bad in forbidden:
                if bad in text:
                    violations.append(f"[{name}] {p} contains forbidden pattern: {bad}")
                    break

    return name, violations


def main() -> None:
    parser = argparse.ArgumentParser(description="Architecture lint checker (config-driven).")
    parser.add_argument("--config", type=str, default=None, help="Path to lint rules JSON (default: sots_architecture_lint.json).")
    args = parser.parse_args()

    confirm_start("architecture_lint")

    tools_root = Path(__file__).resolve().parent
    project_root = get_project_root()

    if args.config:
        cfg_path = Path(args.config).resolve()
    else:
        cfg_path = tools_root / "sots_architecture_lint.json"

    print(f"[INFO] Using rules config: {cfg_path}")

    cfg = load_rules(cfg_path)
    rules = cfg.get("rules", [])

    violations_by_rule: Dict[str, List[str]] = {}
    all_violations: List[str] = []
    touched_files: set[Path] = set()

    for rule in rules:
        name, vs = scan_rule(rule, project_root)
        violations_by_rule[name] = vs
        all_violations.extend(vs)

    for rel in cfg.get("scan_roots", []):
        base = project_root / rel
        if not base.exists():
            continue
        for p in base.rglob("*"):
            if p.is_file() and p.suffix.lower() in (".h", ".cpp", ".cs"):
                touched_files.add(p)

    files_scanned = len(touched_files)

    if all_violations:
        print("\n[VIOLATIONS]")
        for v in all_violations:
            print(f"  {v}")
    else:
        print("\n[VIOLATIONS] (0) no findings")
        print("[INFO] No architecture violations found.")

    print("\n[SUMMARY]")
    print(f"  Rules checked: {len(rules)}")
    print(f"  Files scanned (approx): {files_scanned}")
    print(f"  Violations: {len(all_violations)}")
    for name, vs in violations_by_rule.items():
        print(f"    - {name}: {len(vs)} violations")

    print_llm_summary(
        "architecture_lint",
        CONFIG=str(cfg_path),
        RULE_COUNT=len(rules),
        FILES_SCANNED=files_scanned,
        VIOLATIONS=len(all_violations),
        VIOLATIONS_PER_RULE=json.dumps({name: len(vs) for name, vs in violations_by_rule.items()}),
    )

    confirm_exit()


if __name__ == "__main__":
    main()
