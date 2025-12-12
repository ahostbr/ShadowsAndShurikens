from __future__ import annotations
import argparse
from pathlib import Path
import json

from project_paths import get_plugins_dir
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def audit_plugins(plugins_dir: Path, show_modules: bool):
    results = []
    for plugin_dir in sorted(p for p in plugins_dir.iterdir() if p.is_dir()):
        name = plugin_dir.name
        has_bins = (plugin_dir / "Binaries").exists()
        has_intermediate = (plugin_dir / "Intermediate").exists()
        uplugin_files = list(plugin_dir.glob("*.uplugin"))
        modules = []
        if show_modules and uplugin_files:
            try:
                data = json.loads(uplugin_files[0].read_text(encoding="utf-8"))
                for m in data.get("Modules", []):
                    modules.append(m.get("Name", ""))
            except Exception:
                pass
        results.append((name, has_bins, has_intermediate, modules))
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description="Audit plugins under Plugins/.")
    parser.add_argument("--show-modules", action="store_true", help="Show module list per plugin.")
    args = parser.parse_args()

    confirm_start("plugin_audit")
    plugins_dir = get_plugins_dir()
    print(f"[INFO] Auditing plugins in: {plugins_dir}")

    results = audit_plugins(plugins_dir, args.show_modules)

    for name, has_bins, has_intermediate, modules in results:
        line = f"- {name}: Binaries={has_bins}, Intermediate={has_intermediate}"
        if args.show_modules and modules:
            line += f", Modules={modules}"
        print(line)

    print_llm_summary(
        "plugin_audit",
        PLUGIN_COUNT=len(results),
        WITH_BINARIES=sum(1 for r in results if r[1]),
        WITH_INTERMEDIATE=sum(1 for r in results if r[2]),
    )

    confirm_exit()


if __name__ == "__main__":
    main()
