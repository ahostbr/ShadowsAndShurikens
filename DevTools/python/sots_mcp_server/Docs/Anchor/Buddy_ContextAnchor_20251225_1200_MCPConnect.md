Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1200 | Plugin: sots_mcp_server | Pass/Topic: MCP_CONNECT | Owner: Buddy
Scope: VS Code MCP stdio launch config hardening

DONE
- Updated MCP config to call server via absolute path: `E:\SAS\ShadowsAndShurikens\DevTools\python\sots_mcp_server\server.py`.

VERIFIED
- Verified the configured venv Python exists and can `import mcp`.

UNVERIFIED / ASSUMPTIONS
- VS Code MCP client successfully spawns and connects to the server after reload.

FILES TOUCHED
- .vscode/mcp.json
- DevTools/python/sots_mcp_server/Docs/Worklogs/BuddyWorklog_20251225_120000_MCPConnect.md
- DevTools/python/sots_mcp_server/Docs/Anchor/Buddy_ContextAnchor_20251225_1200_MCPConnect.md

NEXT (Ryan)
- In VS Code: run “Developer: Reload Window”.
- Trigger MCP connection (whatever command you use in your setup) and confirm tool list populates.
- Confirm new entries appear in `DevTools/python/logs/sots_mcp_server.log` for today.

ROLLBACK
- Revert .vscode/mcp.json to the previous relative `args` value.
