import shutil
from pathlib import Path

WORKSPACE_ROOT = Path(__file__).resolve().parents[2]
PLUGINS_DIR = WORKSPACE_ROOT / "Plugins"
DOCS_ROOT = PLUGINS_DIR / "Docs"

if not DOCS_ROOT.is_dir():
    raise SystemExit("Plugins/Docs directory not found")

plugin_dirs = [d.name for d in PLUGINS_DIR.iterdir() if d.is_dir()]
plugin_dirs.sort(key=len, reverse=True)

MANUAL_ALIASES = {
    "SOTS_GlobalStealthManager": {"SOTS_GSM", "SOTSGlobalStealth", "GSM", "DragonStealth"},
    "SOTS_KillExecutionManager": {"SOTS_KEM", "KEM"},
    "SOTS_AIPerception": {"AIPerc"},
    "SOTS_GAS_Plugin": {"Ability"},
    "SOTS_FX_Plugin": {"FX"},
    "BEP": {"NodeJson"},
    "SOTS_ProfileShared": {"ProfileShared"},
    "SOTS_MissionDirector": {"MissionDirector"},
    "SOTS_Stats": {"StatsSnapshot"},
}

def build_aliases(name: str) -> set[str]:
    aliases = {name, name.replace("_", "")}
    if name.endswith("_Plugin"):
        base = name[: -len("_Plugin")]
        aliases.add(base)
        aliases.add(base.replace("_", ""))
    if name.startswith("SOTS_"):
        rest = name.split("SOTS_", 1)[1]
        aliases.add(rest)
        aliases.add(rest.replace("_", ""))
        for segment in rest.split("_"):
            aliases.add(segment)
    aliases.update(MANUAL_ALIASES.get(name, set()))
    return aliases

alias_map = {plugin: build_aliases(plugin) for plugin in plugin_dirs}

moved = []
skipped = []

for doc in DOCS_ROOT.iterdir():
    if not doc.is_file() or doc.suffix.lower() != ".md":
        continue

    target_plugin = None
    for plugin, aliases in alias_map.items():
        if any(alias in doc.name for alias in aliases):
            target_plugin = plugin
            break

    if not target_plugin:
        skipped.append((doc.name, "no matching plugin"))
        continue

    dest_dir = PLUGINS_DIR / target_plugin / "Docs" / "Worklogs"
    dest_dir.mkdir(parents=True, exist_ok=True)
    dest_file = dest_dir / doc.name

    if dest_file.exists():
        skipped.append((doc.name, "destination already exists"))
        continue

    shutil.move(str(doc), str(dest_file))
    moved.append((doc.name, target_plugin))

print("Moved files:")
for name, plugin in moved:
    print(f"  {name} -> {plugin}")

if skipped:
    print("\nSkipped files:")
    for name, reason in skipped:
        print(f"  {name}: {reason}")
