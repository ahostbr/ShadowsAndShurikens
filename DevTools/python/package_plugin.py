from __future__ import annotations
import argparse
from pathlib import Path
import zipfile

from project_paths import get_plugins_dir
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def package_plugin(plugin_name: str, output_dir: Path) -> Path | None:
    plugins_dir = get_plugins_dir()
    plugin_dir = plugins_dir / plugin_name
    if not plugin_dir.exists():
        print(f"[ERROR] Plugin folder not found: {plugin_dir}")
        return None

    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / f"{plugin_name}.zip"

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for path in plugin_dir.rglob("*"):
            rel = path.relative_to(plugin_dir)
            zf.write(path, arcname=str(rel).replace("\\", "/"))

    return zip_path


def main() -> None:
    parser = argparse.ArgumentParser(description="Package a plugin folder into a zip.")
    parser.add_argument("plugin", type=str, help="Plugin name (folder/.uplugin name).")
    parser.add_argument("--output-dir", type=str, default="DevTools/PluginZips", help="Output directory for zips.")
    args = parser.parse_args()

    confirm_start("package_plugin")

    output_dir = Path(args.output_dir).resolve()
    zip_path = package_plugin(args.plugin, output_dir)

    if zip_path is None:
        print_llm_summary(
            "package_plugin",
            PLUGIN=args.plugin,
            SUCCESS=False,
        )
    else:
        print(f"[INFO] Created: {zip_path}")
        print_llm_summary(
            "package_plugin",
            PLUGIN=args.plugin,
            SUCCESS=True,
            ZIP=str(zip_path),
        )

    confirm_exit()


if __name__ == "__main__":
    main()
