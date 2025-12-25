# Buddy Worklog — MCPConnectPy312 (2025-12-25 09:49)

## Goal
- Get VS Code MCP (stdio) connecting to the SOTS MCP server reliably.

## What changed
- Created a new stable Python 3.12 virtual environment for MCP usage.
- Installed MCP + OpenAI Agents deps into that venv.
- Updated VS Code MCP config to point at the new interpreter.

## Files changed
- .vscode/mcp.json

## Notes / Risks / Unknowns
- Root cause of previous failure: the configured venv was using Python 3.15 alpha, which cannot load `pydantic_core` wheels required by `pydantic`/`mcp`.
- `run_mcp_list_tools.py` appears to hang when using the MCP Python stdio client on this machine; manual JSON-RPC over stdio works (server responds), suggesting a client-side issue or a Windows stdio edge-case.
- MCP Inspector (`mcp dev`) crashes due to a dataclass import/load issue in the inspector’s loader path (server imports fine in normal execution). This is separate from VS Code MCP usage.

## Verification status
- VERIFIED (local, via manual stdio JSON-RPC): `initialize` and `tools/list` return valid responses from `DevTools/python/sots_mcp_server/server.py`.
- UNVERIFIED: VS Code UI connection state (not observable from here).

## Follow-ups / Next steps
- In VS Code, reload window to ensure `.vscode/mcp.json` is reloaded, then confirm tools are available from the MCP integration.
- If the helper script is still needed, consider adding explicit timeouts / stderr capture to it (would require approval to modify DevTools).
