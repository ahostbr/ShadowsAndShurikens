# BPGen Getting Started (SPINE_P)

Use this as the quick-start for a clean BPGen + Bridge setup (mirrors VibeUE flow, names swapped to BPGen).

## 1) Enable Plugins
- Enable `SOTS_BlueprintGen` and `SOTS_BPGen_Bridge` in the Editor (Editor-only modules). Keep `SOTS_BPGen_Bridge` disabled for packaged builds.

## 2) Start the Bridge (Editor)
- Open the tab `Window → SOTS Tools → SOTS BPGen Control Center`.
- Click **Start Bridge**; confirm status shows running and bind `127.0.0.1:55557`.
- Safe mode defaults to ON; dangerous ops require `dangerous_ok=true`.

## 3) Run MCP Server (Python)
- From repo root: `cd DevTools/python/sots_mcp_server && python server.py`
- Ensure MCP points at the bridge host/port (defaults match 127.0.0.1:55557).

## 4) Run Example 01 (Print String)
- Copy `Templates/GraphSpecs/function_print_string.json` and set `BlueprintAssetPath` + `FunctionName`.
- Use MCP action `apply_graph_spec` with the template payload.
- Refresh Control Center recents to verify request shows ok=true.

## 5) Verify / Compile / Save
- In the Editor, open the target Blueprint and compile/save manually (or via MCP `compile_blueprint` if exposed).

## 6) Troubleshooting
- `ping` or `server_info` via bridge to confirm protocol_version 1.0.
- If applies are blocked: set env `SOTS_ALLOW_APPLY=1` for the editor session.
- If safe_mode blocks delete/replace: add `dangerous_ok=true` in params or turn off safe mode (not recommended).
- If rate-limited: adjust MaxRequestsPerSecond/Minute in bridge config (see Distribution toggles).

## 7) Updating / Migration
- Check `Docs/API/BPGen_ResultSchemas.md` for spec_version.
- Run `canonicalize_graph_spec` / `canonicalize_spec` before apply if templates are older.
