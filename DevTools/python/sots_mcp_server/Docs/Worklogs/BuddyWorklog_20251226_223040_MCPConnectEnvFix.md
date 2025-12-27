# Buddy Worklog — 2025-12-26 22:30:40 — MCPConnectEnvFix

## Goal
Make `SOTS_MCP` connect reliably by ensuring VS Code passes the correct env var names expected by the MCP server.

## What changed
- Updated `.vscode/mcp.json` to use `SOTS_ALLOW_BPGEN_APPLY` (the server’s expected gate) instead of `SOTS_BPGEN_APPLY`.

## Files changed
- .vscode/mcp.json

## Notes / Risks / Unknowns
- UNVERIFIED: I did not restart VS Code MCP servers here.
- This change only affects BPGen apply gating; base MCP connection should work regardless, but tool behavior will now match intended config.

## Verification status
- Static: JSON has no VS Code diagnostics.

## Next steps (Ryan)
- In VS Code, reload MCP servers (or reload window) and confirm `SOTS_MCP` starts.
- If BPGen calls still fail, check the UE bridge is running on `127.0.0.1:55557`.
