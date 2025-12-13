from __future__ import annotations

import argparse
import json
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
import shutil
from typing import Any, Dict, List, Optional

try:
    from project_paths import get_project_root  # type: ignore
except Exception:
    get_project_root = None  # type: ignore


@dataclass
class ConfigReport:
    path: Path
    source_type: str  # json / uasset / other
    parsed: bool
    detail_lines: List[str] = field(default_factory=list)
    discovery_missing: bool = False
    weather_missing: bool = False
    sun_missing: bool = False
    dlwe_surface_missing: bool = False


# -------------- helpers -----------------

def find_project_root(cli_project: Optional[str]) -> Path:
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
    # Write to DevTools/logs by default (not python/logs)
    out_dir = tools_root.parent / "logs"
    out_dir.mkdir(parents=True, exist_ok=True)
    return out_dir / f"udsbridge_audit_{ts}.txt"


def update_latest_snapshot(log_path: Path) -> Path:
    """
    Maintain a latest pointer for easy diffing. Prefer a symlink; fall back to copying.
    """
    latest = log_path.parent / "udsbridge_audit_latest.txt"
    try:
        if latest.exists() or latest.is_symlink():
            latest.unlink()
    except Exception:
        pass

    try:
        latest.symlink_to(log_path)
        return latest
    except Exception:
        try:
            shutil.copyfile(log_path, latest)
            return latest
        except Exception:
            return log_path


def discover_candidates(project_root: Path) -> List[Path]:
    roots = [
        project_root / "Content",
        project_root / "BEP_EXPORTS",
        project_root / "EXPORTS",
        project_root / "IMPORTS",
    ]
    exts = {".json", ".uasset"}
    tokens = ["udsbridge", "uds_bridge"]
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
                stem = path.stem.lower()
                if any(tok in stem for tok in tokens):
                    candidates.append(path)
                    continue
                if path.suffix.lower() == ".json":
                    try:
                        sample = path.read_text(encoding="utf-8", errors="ignore")[:2048].lower()
                        if "udsbridge" in sample:
                            candidates.append(path)
                    except Exception:
                        continue
        except Exception:
            continue
    return sorted(set(candidates))


def read_json(path: Path) -> Dict[str, Any] | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None


def get_properties(blob: Dict[str, Any]) -> Dict[str, Any]:
    if not isinstance(blob, dict):
        return {}
    if "Properties" in blob and isinstance(blob["Properties"], dict):
        return blob["Properties"]
    return blob


def extract_class_name(blob: Dict[str, Any]) -> str:
    for key in ("Type", "NativeClass", "Class", "BlueprintType"):
        val = blob.get(key)
        if isinstance(val, str):
            return val
    return ""


def value_as_text(val: Any) -> str:
    if isinstance(val, str):
        return val.strip()
    if isinstance(val, dict):
        for k in ("AssetPathName", "ObjectPath", "Path", "PackageName", "PackagePath", "Name"):
            inner = val.get(k)
            if isinstance(inner, str) and inner.strip():
                return inner.strip()
    return ""


def prop_text(props: Dict[str, Any], *keys: str) -> str:
    for key in keys:
        if key in props:
            txt = value_as_text(props[key])
            if txt:
                return txt
    return ""


def prop_bool(props: Dict[str, Any], *keys: str) -> Optional[bool]:
    for key in keys:
        if key not in props:
            continue
        val = props[key]
        if isinstance(val, bool):
            return val
        if isinstance(val, (int, float)):
            return bool(val)
        if isinstance(val, str):
            lowered = val.strip().lower()
            if lowered in {"true", "1", "yes", "on"}:
                return True
            if lowered in {"false", "0", "no", "off"}:
                return False
    return None


def status_line(label: str, ok: bool, warn_text: str) -> str:
    return f"  - {label}: {'OK' if ok else f'WARN: {warn_text}'}"


def status_line_info(label: str, ok: bool, missing_text: str) -> str:
    return f"  - {label}: {'OK' if ok else f'INFO: {missing_text}'}"


def info_line(label: str, text: str) -> str:
    return f"  - {label}: {text}"


def evaluate_json_config(path: Path, data: Dict[str, Any]) -> ConfigReport:
    props = get_properties(data)
    class_name = extract_class_name(data).lower()
    is_bridge_class = "udsbridgeconfig" in class_name if class_name else False
    report = ConfigReport(path=path, source_type="json", parsed=True)

    # Discovery
    soft_ref = prop_text(props, "UDSActorSoftRef")
    tag = prop_text(props, "UDSActorTag")
    name_contains = prop_text(props, "UDSActorNameContains")

    report.detail_lines.append("UDS discovery:")
    report.detail_lines.append(status_line("UDSActorSoftRef", bool(soft_ref), "not set"))
    report.detail_lines.append(status_line_info("UDSActorTag", bool(tag), "not set"))
    report.detail_lines.append(status_line_info("UDSActorNameContains", bool(name_contains), "not set"))
    if not (soft_ref or tag or name_contains):
        report.detail_lines.append("  - WARN: No UDSActor discovery set")
        report.discovery_missing = True

    # Weather mapping
    snow_prop = prop_text(props, "Weather_bSnowy_Property", "WeatherProperty_Snowy", "WeatherPropertyPath_Snowy", "WeatherPath_Snowy")
    rain_prop = prop_text(props, "Weather_bRaining_Property", "WeatherProperty_Raining", "WeatherPropertyPath_Raining", "WeatherPath_Raining")
    dust_prop = prop_text(props, "Weather_bDusty_Property", "WeatherProperty_Dusty", "WeatherPropertyPath_Dusty", "WeatherPath_Dusty")

    report.detail_lines.append("Weather mapping:")
    report.detail_lines.append(status_line("Snowy", bool(snow_prop), "Snow mapping missing"))
    report.detail_lines.append(status_line("Raining", bool(rain_prop), "Rain mapping missing"))
    report.detail_lines.append(status_line("Dusty", bool(dust_prop), "Dust mapping missing"))
    if not (snow_prop and rain_prop and dust_prop):
        report.weather_missing = True

    # Sun direction
    sun_light = prop_text(props, "SunLightActorProperty")
    sun_fn = prop_text(props, "SunDirectionFunctionName")
    sun_rot = prop_text(props, "SunWorldRotationProperty")

    report.detail_lines.append("Sun direction:")
    sun_ok = bool(sun_light or sun_fn or sun_rot)
    report.detail_lines.append(status_line("Sun mapping", sun_ok, "No sun direction mapping configured"))
    if not sun_ok:
        report.sun_missing = True

    # DLWE surface
    settings_fn = prop_text(props, "DLWE_Func_SetInteractionSettings")
    snow_fn = prop_text(props, "DLWE_EnableSnow_Function", "DLWE_Func_SetSnowEnabled")
    rain_fn = prop_text(props, "DLWE_EnableRain_Function", "DLWE_Func_SetRainEnabled", "DLWE_Func_SetPuddlesEnabled")
    dust_fn = prop_text(props, "DLWE_EnableDust_Function", "DLWE_Func_SetDustEnabled")

    apply_surface_ok = bool(settings_fn or snow_fn or rain_fn or dust_fn)
    report.detail_lines.append("DLWE surface:")
    report.detail_lines.append(status_line("Apply surface", apply_surface_ok, "No settings/toggle functions set"))

    snow_asset = prop_text(props, "DLWE_Settings_Snow")
    clear_asset = prop_text(props, "DLWE_Settings_Clear")
    rain_asset = prop_text(props, "DLWE_Settings_Rain")
    dust_asset = prop_text(props, "DLWE_Settings_Dust")

    report.detail_lines.append(status_line("Settings_Snow", bool(snow_asset), "not assigned"))
    report.detail_lines.append(status_line("Settings_Clear", bool(clear_asset), "not assigned"))
    report.detail_lines.append(status_line("Settings_Rain", bool(rain_asset), "not assigned"))
    report.detail_lines.append(status_line("Settings_Dust", bool(dust_asset), "not assigned"))

    if not apply_surface_ok:
        report.dlwe_surface_missing = True

    if not is_bridge_class:
        report.detail_lines.append("  - INFO: Asset class not labeled UDSBridgeConfig; auditing fields by best effort.")

    return report


def evaluate_uasset(path: Path) -> ConfigReport:
    report = ConfigReport(path=path, source_type="uasset", parsed=False)
    report.detail_lines.append("Binary .uasset not parsed; export to JSON to inspect values.")
    report.discovery_missing = True
    report.weather_missing = True
    report.sun_missing = True
    report.dlwe_surface_missing = True
    return report


def build_report(results: List[ConfigReport], project_root: Path) -> str:
    lines: List[str] = []
    lines.append("SOTS UDSBridge Config Audit")
    lines.append(f"Project: {project_root}")
    lines.append(f"Configs found: {len(results)}")
    lines.append("")

    for res in results:
        lines.append(str(res.path))
        lines.append(f"  Source: {res.source_type} | Parsed: {res.parsed}")
        for dl in res.detail_lines:
            lines.append(dl)
        lines.append("")

    parsed = [r for r in results if r.parsed]
    discovery_missing = sum(1 for r in results if r.discovery_missing)
    weather_missing = sum(1 for r in results if r.weather_missing)
    sun_missing = sum(1 for r in results if r.sun_missing)
    dlwe_missing = sum(1 for r in results if r.dlwe_surface_missing)

    lines.append("Summary:")
    lines.append(f"  Configs found: {len(results)}")
    lines.append(f"  Discovery unconfigured: {discovery_missing}")
    lines.append(f"  Weather mappings missing: {weather_missing}")
    lines.append(f"  Sun mapping missing: {sun_missing}")
    lines.append(f"  DLWE apply surface missing: {dlwe_missing}")
    if results and len(parsed) != len(results):
        lines.append(f"  Note: {len(results) - len(parsed)} config(s) not parsed (binary); counts treat them as missing where applicable.")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit USOTS_UDSBridgeConfig assets for discovery/mapping coverage.")
    parser.add_argument("--project", help="Project root (defaults to detected root)")
    parser.add_argument("--out", help="Output report path (defaults to DevTools/logs/udsbridge_audit_<timestamp>.txt)")
    args = parser.parse_args()

    tools_root = Path(__file__).resolve().parent
    project_root = find_project_root(args.project)

    log_path = Path(args.out).resolve() if args.out else default_log_path(tools_root)
    log_path.parent.mkdir(parents=True, exist_ok=True)

    try:
        candidates = discover_candidates(project_root)
    except Exception as exc:
        msg = f"[ERROR] Failed to search for configs: {exc}"
        print(msg)
        log_path.write_text(msg + "\n", encoding="utf-8")
        print(f"UDSBridge audit report written to: {log_path}")
        return 1

    if not candidates:
        msg = f"[WARN] No UDSBridge config assets found under {project_root}."
        print(msg)
        log_path.write_text(msg + "\nNothing to audit.\n", encoding="utf-8")
        print(f"UDSBridge audit report written to: {log_path}")
        return 0

    results: List[ConfigReport] = []
    for path in candidates:
        if path.suffix.lower() == ".json":
            data = read_json(path)
            if data is None:
                report = ConfigReport(path=path, source_type="json", parsed=False)
                report.detail_lines.append("Failed to parse JSON; cannot audit fields.")
                report.discovery_missing = True
                report.weather_missing = True
                report.sun_missing = True
                report.dlwe_surface_missing = True
                results.append(report)
                continue
            results.append(evaluate_json_config(path, data))
        elif path.suffix.lower() == ".uasset":
            results.append(evaluate_uasset(path))
        else:
            report = ConfigReport(path=path, source_type=path.suffix.lower().lstrip('.'), parsed=False)
            report.detail_lines.append("Unsupported file type; skipped detailed parsing.")
            results.append(report)

    report_text = build_report(results, project_root)
    log_path.write_text(report_text + "\n", encoding="utf-8")
    latest_path = update_latest_snapshot(log_path)

    parsed = [r for r in results if r.parsed]
    summary_line = (
        f"Configs={len(results)} Parsed={len(parsed)} | "
        f"DiscoveryMissing={sum(1 for r in results if r.discovery_missing)} | "
        f"WeatherMissing={sum(1 for r in results if r.weather_missing)} | "
        f"SunMissing={sum(1 for r in results if r.sun_missing)} | "
        f"DLWEApplyMissing={sum(1 for r in results if r.dlwe_surface_missing)}"
    )
    print(summary_line)
    print(f"UDSBridge audit report written to: {log_path}")
    if latest_path != log_path:
        print(f"Latest snapshot: {latest_path}")
    print("Tip: use diff to compare this report with udsbridge_audit_latest.txt to see config drift.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
