# SOTS MCP Healthcheck

Goal: confirm the single SOTS MCP server is reachable and list core tools.

1) Call MCP tool: `sots_server_info`
2) Call MCP tool: `sots_help`
3) Call MCP tool: `sots_agents_health` (if present)
4) If any tool fails:
   - show the error
   - recommend checking the MCP server entrypoint + environment variables
