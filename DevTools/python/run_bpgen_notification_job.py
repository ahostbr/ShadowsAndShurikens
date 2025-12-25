from pathlib import Path
import json
import sys
root = Path(__file__).resolve().parents[1]
sys.path.append(str(root / "DevTools" / "python"))
from sots_mcp_server import server

def _dump(title: str, payload: dict) -> None:
    print(f"\n--- {title} ---")
    print(json.dumps(payload, ensure_ascii=False, indent=2))

BLUEPRINT_BASE = "/Game/BPGen/BP_BPGen_Testing"
ALIAS = BLUEPRINT_BASE.split("/")[-1]
BLUEPRINT_FULL = f"{BLUEPRINT_BASE}.{ALIAS}"
FUNCTION_NAME = "EventGraph"
GRAPH_SPEC_PATH = "Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_GraphSpec.json"

create_result = server.bpgen_call_tool(
    action="create_blueprint_asset",
    params={
        "asset_path": BLUEPRINT_BASE,
        "AssetPath": BLUEPRINT_BASE,
        "parent_class_path": "/Script/Engine.Actor",
        "ParentClassPath": "/Script/Engine.Actor",
    },
)
_dump("create_blueprint_asset", create_result)

skeleton_result = server.bpgen_call_tool(
    action="apply_function_skeleton",
    params={
        "TargetBlueprintPath": BLUEPRINT_FULL,
        "FunctionName": FUNCTION_NAME,
    },
)
_dump("apply_function_skeleton", skeleton_result)

apply_result = server.bpgen_call_tool(
    action="apply_graph_spec",
    params={
        "blueprint_asset_path": BLUEPRINT_FULL,
        "function_name": FUNCTION_NAME,
        "graph_spec": GRAPH_SPEC_PATH,
    },
)
_dump("apply_graph_spec", apply_result)
