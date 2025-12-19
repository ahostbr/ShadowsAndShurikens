"""FastMCP server exposing BPGen actions over the local TCP bridge (SPINE_G).

Usage:
  python sots_bpgen_mcp_server.py
Environment:
  SOTS_BPGEN_HOST (default 127.0.0.1)
  SOTS_BPGEN_PORT (default 55557)
  SOTS_BPGEN_TIMEOUT (default 30)
"""

from __future__ import annotations

import os
import sys
from typing import Any, Dict

try:
    from mcp.server.fastmcp import FastMCP
except Exception as exc:  # pragma: no cover - import guard
    sys.stderr.write(f"FastMCP not available: {exc}\n")
    sys.stderr.write("Install with: pip install mcp-server==0.0.7 (or matching workspace version)\n")
    sys.exit(1)

from sots_bpgen_bridge_client import bpgen_call

HOST = os.environ.get("SOTS_BPGEN_HOST", "127.0.0.1")
PORT = int(os.environ.get("SOTS_BPGEN_PORT", "55557"))
TIMEOUT = float(os.environ.get("SOTS_BPGEN_TIMEOUT", "30"))


def _call(action: str, params: Dict[str, Any] | None = None) -> Dict[str, Any]:
    return bpgen_call(action, params or {}, host=HOST, port=PORT, recv_timeout=TIMEOUT)


def build_server() -> FastMCP:
    mcp = FastMCP("SOTS_BPGen")

    @mcp.tool()
    def manage_bpgen(action: str, params: Dict[str, Any] | None = None) -> Dict[str, Any]:
        """Primary BPGen tool: forwards actions to the UE bridge."""
        return _call(action, params)

    @mcp.tool()
    def bpgen_ping() -> Dict[str, Any]:
        return _call("ping", {})

    @mcp.tool()
    def bpgen_discover(blueprint_asset_path: str = "", search_text: str = "", max_results: int = 200, include_pins: bool = False) -> Dict[str, Any]:
        return _call(
            "discover_nodes",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "search_text": search_text,
                "max_results": max_results,
                "include_pins": include_pins,
            },
        )

    @mcp.tool()
    def bpgen_apply(blueprint_asset_path: str, function_name: str, graph_spec: Dict[str, Any]) -> Dict[str, Any]:
        return _call(
            "apply_graph_spec",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "function_name": function_name,
                "graph_spec": graph_spec,
            },
        )

    @mcp.tool()
    def bpgen_list(blueprint_asset_path: str, function_name: str, include_pins: bool = False) -> Dict[str, Any]:
        return _call(
            "list_nodes",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "function_name": function_name,
                "include_pins": include_pins,
            },
        )

    @mcp.tool()
    def bpgen_describe(blueprint_asset_path: str, function_name: str, node_id: str, include_pins: bool = True) -> Dict[str, Any]:
        return _call(
            "describe_node",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "function_name": function_name,
                "node_id": node_id,
                "include_pins": include_pins,
            },
        )

    @mcp.tool()
    def bpgen_compile(blueprint_asset_path: str) -> Dict[str, Any]:
        return _call("compile_blueprint", {"blueprint_asset_path": blueprint_asset_path})

    @mcp.tool()
    def bpgen_save(blueprint_asset_path: str) -> Dict[str, Any]:
        return _call("save_blueprint", {"blueprint_asset_path": blueprint_asset_path})

    @mcp.tool()
    def bpgen_refresh(blueprint_asset_path: str, function_name: str, include_pins: bool = False) -> Dict[str, Any]:
        return _call(
            "refresh_nodes",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "function_name": function_name,
                "include_pins": include_pins,
            },
        )

    @mcp.tool()
    def bpgen_delete_node(blueprint_asset_path: str, function_name: str, node_id: str, compile: bool = True, dry_run: bool = False) -> Dict[str, Any]:
        return _call(
            "delete_node_by_id",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "function_name": function_name,
                "node_id": node_id,
                "compile": compile,
                "dry_run": dry_run,
            },
        )

    @mcp.tool()
    def bpgen_delete_link(
        blueprint_asset_path: str,
        function_name: str,
        from_node_id: str,
        from_pin: str,
        to_node_id: str,
        to_pin: str,
        compile: bool = True,
        dry_run: bool = False,
    ) -> Dict[str, Any]:
        return _call(
            "delete_link",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "function_name": function_name,
                "from_node_id": from_node_id,
                "from_pin": from_pin,
                "to_node_id": to_node_id,
                "to_pin": to_pin,
                "compile": compile,
                "dry_run": dry_run,
            },
        )

    @mcp.tool()
    def bpgen_replace_node(
        blueprint_asset_path: str,
        function_name: str,
        existing_node_id: str,
        new_node: Dict[str, Any],
        pin_remap: Dict[str, str] | None = None,
        compile: bool = True,
        dry_run: bool = False,
    ) -> Dict[str, Any]:
        params = {
            "blueprint_asset_path": blueprint_asset_path,
            "function_name": function_name,
            "existing_node_id": existing_node_id,
            "new_node": new_node,
            "compile": compile,
            "dry_run": dry_run,
        }
        if pin_remap:
            params["pin_remap"] = pin_remap
        return _call("replace_node", params)

    @mcp.tool()
    def bpgen_ensure_function(blueprint_asset_path: str, function_name: str, signature: Dict[str, Any] | None = None, create_if_missing: bool = True, update_if_exists: bool = True) -> Dict[str, Any]:
        """Forward to bridge ensure_function if available; otherwise returns bridge error."""
        return _call(
            "ensure_function",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "function_name": function_name,
                "signature": signature or {},
                "create_if_missing": create_if_missing,
                "update_if_exists": update_if_exists,
            },
        )

    @mcp.tool()
    def bpgen_ensure_variable(blueprint_asset_path: str, variable_name: str, pin_type: Dict[str, Any], default_value: Any | None = None, create_if_missing: bool = True, update_if_exists: bool = True, instance_editable: bool = True, expose_on_spawn: bool = False) -> Dict[str, Any]:
        """Forward to bridge ensure_variable if available; otherwise returns bridge error."""
        return _call(
            "ensure_variable",
            {
                "blueprint_asset_path": blueprint_asset_path,
                "variable_name": variable_name,
                "pin_type": pin_type,
                "default_value": default_value,
                "create_if_missing": create_if_missing,
                "update_if_exists": update_if_exists,
                "instance_editable": instance_editable,
                "expose_on_spawn": expose_on_spawn,
            },
        )

    return mcp


def main() -> None:
    server = build_server()
    server.run()


if __name__ == "__main__":
    main()
