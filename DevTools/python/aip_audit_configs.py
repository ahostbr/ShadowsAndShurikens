from __future__ import annotations

import argparse
import json
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, List, Tuple

try:
    from project_paths import get_project_root  # type: ignore
except Exception:
    get_project_root = None  # type: ignore


@dataclass
class Issue:
    level: str  # ERROR / WARN / INFO
    message: str


@dataclass
class AuditResult:
    path: Path
    source_type: str  # json / uasset / other
    issues: List[Issue] = field(default_factory=list)
    notes: List[str] = field(default_factory=list)

    def add(self, level: str, message: str) -> None:
        self.issues.append(Issue(level=level, message=message))

    def has_errors(self) -> bool:
        return any(i.level == "ERROR" for i in self.issues)

    def has_warns(self) -> bool:
        return any(i.level == "WARN" for i in self.issues)


# ----------------------------- helpers ------------------------------------

def normalize_key(key: str) -> str:
    return "".join(ch.lower() for ch in key if ch.isalnum())


def flatten_dict(data: Any, out: Dict[str, Any]) -> None:
    if isinstance(data, dict):
        for k, v in data.items():
            nk = normalize_key(str(k))
            if nk and nk not in out:
                out[nk] = v
            flatten_dict(v, out)
    elif isinstance(data, list):
        for item in data:
            flatten_dict(item, out)


def coerce_bool(value: Any) -> bool | None:
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return bool(value)
    if isinstance(value, str):
        val = value.strip().lower()
        if val in {"true", "1", "yes", "on"}:
            return True
        if val in {"false", "0", "no", "off"}:
            return False
    return None


def coerce_int(value: Any) -> int | None:
    if isinstance(value, bool):  # avoid bool -> int
        return int(value)
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return int(value)
    if isinstance(value, str):
        try:
            return int(float(value))
        except Exception:
            return None
    return None


def coerce_float(value: Any) -> float | None:
    if isinstance(value, bool):
        return float(value)
    if isinstance(value, (int, float)):
        return float(value)
    if isinstance(value, str):
        try:
            return float(value)
        except Exception:
            return None
    return None


def coerce_list(value: Any) -> List[str] | None:
    if isinstance(value, list):
        result: List[str] = []
        for item in value:
            if isinstance(item, str):
                result.append(item)
            else:
                result.append(str(item))
        return result
    if isinstance(value, str):
        parts = [p.strip() for p in value.split(",") if p.strip()]
        return parts
    return None


def load_json(path: Path) -> Tuple[Dict[str, Any] | None, str | None]:
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f), None
    except Exception as exc:
        return None, str(exc)


def find_project_root(cli_project: str | None) -> Path:
    if cli_project:
        return Path(cli_project).resolve()
    if get_project_root is not None:
        try:
            return get_project_root()
        except Exception:
            pass
    return Path.cwd().resolve()


def default_log_path(tools_root: Path) -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = tools_root / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    return log_dir / f"aip_audit_{ts}.txt"


def discover_candidates(project_root: Path) -> List[Path]:
    tokens = ["aiperception", "perceptionconfig", "guardperception"]
    exts = [".uasset", ".json"]
    roots = [project_root / "Content", project_root / "BEP_EXPORTS"]
    candidates: List[Path] = []
    for root in roots:
        if not root.exists():
            continue
        try:
            for path in root.rglob("*"):
                if not path.is_file():
                    continue
                if path.suffix.lower() not in exts:
                    continue
                name = path.stem.lower()
                if any(tok in name for tok in tokens):
                    candidates.append(path)
        except Exception:
            continue
    return sorted(set(candidates))


def evaluate_config(mapping: Dict[str, Any], result: AuditResult) -> None:
    flat: Dict[str, Any] = {}
    flatten_dict(mapping, flat)

    def get_bool(key: str) -> bool | None:
        return coerce_bool(flat.get(key))

    def get_int(key: str) -> int | None:
        return coerce_int(flat.get(key))

    def get_float(key: str) -> float | None:
        return coerce_float(flat.get(key))

    def get_list(key: str) -> List[str] | None:
        return coerce_list(flat.get(key))

    los_enabled = get_bool("benabletargetpointlos")
    target_bones = get_list("targetpointbones")
    max_point_traces = get_int("maxpointtracespertarget")
    max_total_traces = get_int("maxtotaltracesperupdate")
    max_targets = get_int("maxtargetsperupdate")

    shadow_enabled = get_bool("benableshadowawareness")
    shadow_interval = get_float("shadowcheckinterval")
    shadow_traces = get_int("maxshadowtracespersecondperguard")
    shadow_gain = get_float("shadowsuspiciongainpersecond")

    # Target point LOS checks
    if los_enabled is True:
        if target_bones is not None and len(target_bones) == 0:
            result.add("ERROR", "bEnableTargetPointLOS=true but TargetPointBones is empty")
        elif target_bones is None:
            result.add("WARN", "bEnableTargetPointLOS=true but TargetPointBones not found (unable to verify)")
        if max_point_traces is not None and max_point_traces > 6:
            result.add("WARN", f"MaxPointTracesPerTarget={max_point_traces} (>6)")
        if max_total_traces is not None and max_total_traces > 16:
            result.add("WARN", f"MaxTotalTracesPerUpdate={max_total_traces} (>16)")
        if max_targets is not None and max_targets > 4:
            result.add("WARN", f"MaxTargetsPerUpdate={max_targets} (>4)")
    elif los_enabled is None:
        result.notes.append("TargetPoint LOS settings not found")

    # Shadow awareness checks
    if shadow_enabled is True:
        if shadow_interval is not None and shadow_interval < 0.2:
            result.add("WARN", f"ShadowCheckInterval={shadow_interval:.3f} (<0.2)")
        if shadow_traces is not None and shadow_traces > 2:
            result.add("WARN", f"MaxShadowTracesPerSecondPerGuard={shadow_traces} (>2)")
        if shadow_gain is not None and shadow_gain > 0.25:
            result.add("WARN", f"ShadowSuspicionGainPerSecond={shadow_gain:.3f} (>0.25)")
        result.add("INFO", "Shadow awareness enabled; ensure GSM dominant light direction is configured in GlobalStealthManager (not auto-detected).")
    elif shadow_enabled is None:
        result.notes.append("Shadow awareness settings not found")

    if not result.issues and not result.notes:
        result.add("INFO", "No configurable perception fields were detected in this asset (may require JSON export to inspect).")


# ----------------------------- main flow ----------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description="Audit SOTS_AIPerception configs for unsafe budgets and missing fields.")
    parser.add_argument("--project", dest="project", help="Project root (defaults to detected root)")
    parser.add_argument("--out", dest="out", help="Output report path (defaults to DevTools/logs/aip_audit_<timestamp>.txt)")
    args = parser.parse_args()

    tools_root = Path(__file__).resolve().parent
    project_root = find_project_root(args.project)

    log_path = Path(args.out) if args.out else default_log_path(tools_root)
    log_path.parent.mkdir(parents=True, exist_ok=True)

    candidates = discover_candidates(project_root)

    results: List[AuditResult] = []

    if not candidates:
        msg = f"[WARN] No candidate configs found under {project_root} (looked for AIPerception/PerceptionConfig/GuardPerception)."
        print(msg)
        with log_path.open("w", encoding="utf-8") as f:
            f.write(msg + "\n")
            f.write("Nothing to audit.\n")
        print(f"[INFO] Report written to {log_path}")
        return 0

    for path in candidates:
        source_type = path.suffix.lower().lstrip(".")
        result = AuditResult(path=path, source_type=source_type)

        if path.suffix.lower() == ".json":
            data, err = load_json(path)
            if err or data is None:
                result.add("ERROR", f"Failed to parse JSON: {err}")
            else:
                evaluate_config(data, result)
        elif path.suffix.lower() == ".uasset":
            result.add("INFO", "Binary .uasset not parsed; export to JSON to inspect values.")
        else:
            result.add("INFO", "Unsupported file type; skipped detailed parsing.")

        results.append(result)

    errors = sum(1 for r in results if r.has_errors())
    warns = sum(1 for r in results if r.has_warns())

    lines: List[str] = []
    lines.append("SOTS AIPerception Config Audit")
    lines.append(f"Project: {project_root}")
    lines.append(f"Candidates found: {len(results)}")
    lines.append("")

    for res in results:
        lines.append(str(res.path))
        if res.issues:
            for issue in res.issues:
                lines.append(f"  [{issue.level}] {issue.message}")
        if res.notes:
            for note in res.notes:
                lines.append(f"  [INFO] {note}")
        lines.append("")

    lines.append(f"Summary: errors={errors}, warns={warns}, total={len(results)}")

    report_text = "\n".join(lines)

    print(report_text)
    with log_path.open("w", encoding="utf-8") as f:
        f.write(report_text)
        f.write("\n")

    print(f"[INFO] Report written to {log_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
