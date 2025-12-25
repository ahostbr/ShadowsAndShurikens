Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1220 | Plugin: DevTools_sots_mcp_server | Pass/Topic: manage_asset parity wiring | Owner: Buddy
Scope: VibeUE `manage_asset` tool now routes to BPGen bridge `asset_*` actions.

DONE
- Expanded BPGen read-only allowlist to include: `asset_search`, `asset_export_texture`, `asset_list_references`.
- Implemented `manage_asset` actions:
  - `search` -> `asset_search`
  - `open_in_editor` -> `asset_open_in_editor`
  - `duplicate` -> `asset_duplicate`
  - `delete` -> `asset_delete` (adds `dangerous_ok=true`)
  - `import_texture` -> `asset_import_texture`
  - `export_texture` -> `asset_export_texture`
- Implemented `svg_to_png` locally in Python (optional dependency `cairosvg`).

VERIFIED
- `server.py` passes `python -m py_compile`.

UNVERIFIED / ASSUMPTIONS
- Live UE editor + BPGen bridge behavior for each action.
- Whether gating `open_in_editor` behind BPGen apply permission is acceptable vs. upstream expectations.

FILES TOUCHED
- DevTools/python/sots_mcp_server/server.py
- DevTools/python/sots_mcp_server/Docs/Worklogs/BuddyWorklog_20251225_122000_ManageAssetParity.md

NEXT (Ryan)
- With UE running and bridge connected: call `manage_asset` with `search`, then `export_texture` on a known Texture2D.
- Validate `import_texture` from a PNG on disk into `/Game/Textures/Imported`.
- Validate `delete` fails in safe mode and succeeds only when safe mode is disabled (and confirm error codes for in-use assets).

ROLLBACK
- Revert DevTools/python/sots_mcp_server/server.py changes and restart the MCP server.
