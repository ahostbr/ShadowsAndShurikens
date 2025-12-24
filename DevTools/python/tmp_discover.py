import json
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root))
from sots_bpgen_bridge_client import bpgen_call
params = {
    "search_text": "BP_SOTS_Player",
    "max_results": 5,
    "include_pins": True
}
print("payload:", json.dumps(params))
resp = bpgen_call("discover_nodes", params)
print(json.dumps(resp, indent=2))