Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_0949 | Plugin: DevTools_sots_mcp_server | Pass/Topic: MCP Py312 venv + VSCode config | Owner: Buddy
Scope: VS Code MCP stdio config updated to use a stable Python 3.12 MCP venv.

DONE
- Created venv: E:/SAS/ShadowsAndShurikens/.venv_mcp312 (Python 3.12) with `mcp[cli]`, `openai-agents`, `psutil`.
- Updated .vscode/mcp.json server `SOTS.command` to point at .venv_mcp312/Scripts/python.exe.

VERIFIED
- Manual stdio JSON-RPC `initialize` + `tools/list` returns tool list from DevTools/python/sots_mcp_server/server.py.

UNVERIFIED / ASSUMPTIONS
- VS Code MCP UI successfully reconnects after window reload (not observed here).
- `run_mcp_list_tools.py` hang is a client-side stdio issue; server itself responds.

FILES TOUCHED
- .vscode/mcp.json

NEXT (Ryan)
- VS Code: run “Developer: Reload Window”, then verify SOTS MCP tools show up and can call `sots_help`.
- If `run_mcp_list_tools.py` is required for CI/diagnostics, reproduce hang and decide whether to add explicit timeouts/logging.

ROLLBACK
- Revert .vscode/mcp.json to previous interpreter path if needed.
