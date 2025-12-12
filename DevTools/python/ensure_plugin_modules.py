from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Dict, Any, List

from project_paths import get_project_root
from llm_log import print_llm_summary
from cli_utils import confirm_start, confirm_exit


def load_config(cfg_path: Path) -> Dict[str, Any]:
    if not cfg_path.exists():
        print(f"[ERROR] Config file not found: {cfg_path}")
        return {}
    try:
        with cfg_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"[ERROR] Failed to read config '{cfg_path}': {e}")
        return {}


def find_existing_module_cpp(private_dir: Path, module_name: str) -> Path | None:
    """
    Look for any .cpp file in Private that already contains IMPLEMENT_MODULE(..., <ModuleName>).
    """
    if not private_dir.exists():
        return None

    for cpp in private_dir.glob("*.cpp"):
        try:
            text = cpp.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue

        if "IMPLEMENT_MODULE" in text and module_name in text:
            return cpp

    return None


def ensure_module_for_plugin(plugin_root: Path,
                             plugin_desc: Dict[str, Any],
                             default_template: str,
                             dry_run: bool) -> tuple[bool, bool]:
    """
    Ensure that <plugin>/Source/<module>/Private contains a .cpp with IMPLEMENT_MODULE for that module.
    Returns (exists, fixed)
      exists=True  -> module already present
      fixed=True   -> we created/appended a module cpp
    """
    plugin_name: str = plugin_desc.get("plugin_name", "")
    module_name: str = plugin_desc.get("module_name", "")
    log_category: str = plugin_desc.get("log_category", f"Log{module_name}Module")

    if not plugin_name or not module_name:
        print(f"[WARN] Skipping malformed config entry: {plugin_desc}")
        return False, False

    plugin_dir = plugin_root / plugin_name
    if not plugin_dir.exists():
        print(f"[WARN] Plugin folder not found: {plugin_dir}")
        return False, False

    module_source_dir = plugin_dir / "Source" / module_name
    private_dir = module_source_dir / "Private"

    # Sanity: check .uplugin exists and references the module (optional, but useful)
    uplugin = plugin_dir / f"{plugin_name}.uplugin"
    if not uplugin.exists():
        print(f"[WARN] .uplugin not found for {plugin_name}: {uplugin}")

    # First, see if a module file already exists.
    existing_module_cpp = find_existing_module_cpp(private_dir, module_name)
    if existing_module_cpp is not None:
        print(f"[OK] Module already present for {plugin_name} ({module_name}) in {existing_module_cpp}")
        return True, False

    # No existing IMPLEMENT_MODULE – we need to create/append one.
    template: str = plugin_desc.get("template", default_template)
    rendered = template.replace("{MODULE_NAME}", module_name).replace("{LOG_CATEGORY}", log_category)

    # Ensure Private/ exists
    if not private_dir.exists() and not dry_run:
        private_dir.mkdir(parents=True, exist_ok=True)

    target_cpp = private_dir / f"{module_name}Module.cpp"

    if target_cpp.exists():
        # File exists but no IMPLEMENT_MODULE in it – append the module boilerplate.
        msg = f"[FIX] Appending module implementation for {plugin_name} ({module_name}) in {target_cpp}"
        if dry_run:
            print(f"[DRY-RUN] {msg}")
        else:
            print(msg)
            with target_cpp.open("a", encoding="utf-8") as f:
                f.write("\n\n")
                f.write(rendered)
    else:
        # Create new file.
        msg = f"[FIX] Creating module file for {plugin_name} ({module_name}) at {target_cpp}"
        if dry_run:
            print(f"[DRY-RUN] {msg}")
        else:
            print(msg)
            with target_cpp.open("w", encoding="utf-8") as f:
                f.write(rendered)
                f.write("\n")

    return False, not dry_run  # exists=False, fixed=True (unless dry-run)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Ensure each listed plugin has a valid module .cpp with IMPLEMENT_MODULE()."
    )
    parser.add_argument(
        "--config",
        type=str,
        default="sots_plugin_modules.json",
        help="Path to the plugin/module config JSON (default: sots_plugin_modules.json in tools folder).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Only report what would be changed; do not write files.",
    )
    args = parser.parse_args()

    confirm_start("ensure_plugin_modules")

    tools_root = Path(__file__).resolve().parent
    project_root = get_project_root()

    cfg_path = Path(args.config)
    if not cfg_path.is_absolute():
        cfg_path = tools_root / cfg_path

    print(f"[INFO] Tools root:   {tools_root}")
    print(f"[INFO] Project root: {project_root}")
    print(f"[INFO] Config file:  {cfg_path}")
    print(f"[INFO] Dry run:      {args.dry_run}")

    cfg = load_config(cfg_path)
    if not cfg:
        print_llm_summary(
            "ensure_plugin_modules",
            CONFIG=str(cfg_path),
            PLUGINS_TOTAL=0,
            PLUGINS_FIXED=0,
            PLUGINS_OK=0,
            DRY_RUN=args.dry_run,
            ERROR="CONFIG_LOAD_FAILED",
        )
        confirm_exit()
        return

    plugin_root_rel = cfg.get("plugin_root", "Plugins")
    plugin_root = project_root / plugin_root_rel

    plugins: List[Dict[str, Any]] = cfg.get("plugins", [])
    default_template = cfg.get("default_template", {}).get("body", "")

    if not plugins:
        print("[WARN] No plugins listed in config.")
        print_llm_summary(
            "ensure_plugin_modules",
            CONFIG=str(cfg_path),
            PLUGINS_TOTAL=0,
            PLUGINS_FIXED=0,
            PLUGINS_OK=0,
            DRY_RUN=args.dry_run,
            ERROR="NO_PLUGINS",
        )
        confirm_exit()
        return

    if not default_template:
        print("[WARN] No default_template.body in config; using a hardcoded fallback template.")
        default_template = (
            "// Auto-generated by SOTS DevTools: ensure_plugin_modules\n"
            "// Minimal runtime module for {MODULE_NAME}.\n\n"
            "#include \"Modules/ModuleManager.h\"\n"
            "#include \"Logging/LogMacros.h\"\n\n"
            "DEFINE_LOG_CATEGORY_STATIC({LOG_CATEGORY}, Log, All);\n\n"
            "class F{MODULE_NAME}Module : public IModuleInterface\n"
            "{\n"
            "public:\n"
            "    virtual void StartupModule() override\n"
            "    {\n"
            "        UE_LOG({LOG_CATEGORY}, Log, TEXT(\"{MODULE_NAME} module starting up\"));\n"
            "    }\n\n"
            "    virtual void ShutdownModule() override\n"
            "    {\n"
            "        UE_LOG({LOG_CATEGORY}, Log, TEXT(\"{MODULE_NAME} module shutting down\"));\n"
            "    }\n"
            "};\n\n"
            "IMPLEMENT_MODULE(F{MODULE_NAME}Module, {MODULE_NAME});\n"
        )

    print(f"[INFO] Plugin root: {plugin_root}")

    total = 0
    fixed = 0
    ok = 0

    for plugin_desc in plugins:
        total += 1
        exists, did_fix = ensure_module_for_plugin(plugin_root, plugin_desc, default_template, args.dry_run)
        if exists:
            ok += 1
        if did_fix:
            fixed += 1

    print("\n[SUMMARY]")
    print(f"  Plugins checked: {total}")
    print(f"  Plugins already OK: {ok}")
    print(f"  Plugins fixed: {fixed}")
    print(f"  Dry run: {args.dry_run}")

    print_llm_summary(
        "ensure_plugin_modules",
        CONFIG=str(cfg_path),
        PLUGINS_TOTAL=total,
        PLUGINS_OK=ok,
        PLUGINS_FIXED=fixed,
        DRY_RUN=args.dry_run,
    )

    confirm_exit()


if __name__ == "__main__":
    main()
