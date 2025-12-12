from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, Iterable, List

from project_paths import get_project_root
from llm_log import print_llm_summary

CONFIG_PATH = Path(__file__).resolve().parent / "sots_tools_config.json"
FALLBACK_JSON = Path(__file__).resolve().parent / "configs" / "DA_KEM_OmniTraceTuningConfig.json"


def _read_tools_config() -> Dict[str, Any]:
    if not CONFIG_PATH.exists():
        return {}
    try:
        return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        print(f"[WARN] Failed to parse {CONFIG_PATH}")
        return {}


def _get_tuning_asset_path(config: Dict[str, Any]) -> str | None:
    omnitrace = config.get("omnitrace_kem")
    if not isinstance(omnitrace, dict):
        return None
    candidate = omnitrace.get("tuning_config_asset_path")
    return candidate if isinstance(candidate, str) and candidate.strip() else None


def _load_entries_from_unreal(asset_path: str) -> List[Dict[str, Any]] | None:
    try:
        import unreal  # type: ignore
    except ImportError:
        return None

    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        return None

    raw_entries = getattr(asset, "Entries", [])
    entries: List[Dict[str, Any]] = []
    for raw in raw_entries:
        preset = getattr(raw, "PresetId", None)
        entries.append(
            {
                "PresetId": str(preset) if preset is not None else "Unknown",
                "MaxDistanceOverride": getattr(raw, "MaxDistanceOverride", 0.0),
                "MaxVerticalOffsetOverride": getattr(raw, "MaxVerticalOffsetOverride", 0.0),
                "AllowSteepAngles": getattr(raw, "bAllowSteepAngles", False),
            }
        )
    return entries


def _load_entries_from_json(path: Path) -> List[Dict[str, Any]] | None:
    if not path.exists():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        print(f"[WARN] Invalid JSON in {path}")
        return None

    if not isinstance(data, dict):
        return None

    raw_entries = data.get("Entries")
    if not isinstance(raw_entries, list):
        return None

    entries: List[Dict[str, Any]] = []
    for raw in raw_entries:
        if not isinstance(raw, dict):
            continue
        entries.append(
            {
                "PresetId": raw.get("PresetId", "Unknown"),
                "MaxDistanceOverride": raw.get("MaxDistanceOverride", 0.0),
                "MaxVerticalOffsetOverride": raw.get("MaxVerticalOffsetOverride", 0.0),
                "AllowSteepAngles": raw.get("bAllowSteepAngles", False),
            }
        )
    return entries


def _resolve_fallback_json() -> Path:
    project_root = get_project_root()
    explicit = FALLBACK_JSON
    if explicit.exists():
        return explicit
    # Derive from asset path (optional)
    return explicit


def _format_report(entries: Iterable[Dict[str, Any]], asset_path: str | None) -> str:
    lines = ["[OmniTrace KEM Tuning Report]", ""]
    if asset_path:
        lines.append(f"Asset: {asset_path}")
        lines.append("")
    for entry in entries:
        preset = entry.get("PresetId", "Unknown")
        lines.append(f"Preset: {preset}")
        max_dist = entry.get("MaxDistanceOverride", 0.0)
        vert = entry.get("MaxVerticalOffsetOverride", 0.0)
        allow = entry.get("AllowSteepAngles", False)
        lines.append(f"  MaxDistanceOverride = {max_dist}")
        lines.append(f"  MaxVerticalOffsetOverride = {vert}")
        lines.append(f"  AllowSteepAngles = {allow}")
    lines.append("")
    lines.append("NOTE: Run inside Unreal for the authoritative asset contents.")
    return "\n".join(lines)


def _report(entries: List[Dict[str, Any]], asset_path: str | None) -> None:
    summary = _format_report(entries, asset_path)
    print(summary)
    print_llm_summary(
        "kem_omnitrace_tuning_report",
        status="OK" if entries else "WARN",
        asset_path=asset_path,
        entry_count=len(entries),
        summary=summary,
    )


def main() -> None:
    config = _read_tools_config()
    asset_path = _get_tuning_asset_path(config)

    entries: List[Dict[str, Any]] | None = None
    if asset_path:
        entries = _load_entries_from_unreal(asset_path)

    if entries is None or not entries:
        fallback = _resolve_fallback_json()
        entries = _load_entries_from_json(fallback)

    if not entries:
        msg = "[WARN] No OmniTrace tuning entries were found."
        print(msg)
        print_llm_summary("kem_omnitrace_tuning_report", status="WARN", summary=msg)
        return

    _report(entries, asset_path)


if __name__ == "__main__":
    main()
