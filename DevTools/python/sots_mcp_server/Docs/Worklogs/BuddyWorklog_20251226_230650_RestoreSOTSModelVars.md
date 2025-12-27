# Buddy Worklog — 2025-12-26 23:06:50 — RestoreSOTSModelVars

## Goal
Restore the `SOTS_MODEL_*` environment entries in VS Code MCP config to the requested alias values.

## What changed
- Added/Restored these entries under `SOTS_MCP.env`:
  - `SOTS_MODEL_TRIAGE=GPT_5_2_MINI`
  - `SOTS_MODEL_CODE=GPT_5_2_CODEX`
  - `SOTS_MODEL_DEVTOOLS=GPT_5_2`
  - `SOTS_MODEL_EDITOR=GPT_5_2_CODEX`

## Files changed
- .vscode/mcp.json

## Verification
- UNVERIFIED: did not restart VS Code MCP servers.
- Static: JSON has no VS Code diagnostics.

## Next steps (Ryan)
- Restart MCP servers in VS Code so the new env vars take effect.
