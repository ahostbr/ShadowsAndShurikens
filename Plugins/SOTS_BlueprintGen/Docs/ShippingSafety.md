# Shipping/Test Safety — SOTS_BlueprintGen

- Module type: Editor-only. Do not add runtime dependencies or references into game modules.
- Apply/mutation guarded by env `SOTS_ALLOW_APPLY=1` (used by `USOTS_BPGenAPI` and MCP tools).
- No logs at Shipping/Test: keep verbose logging disabled or behind `UE_BUILD_DEBUG/DEVELOPMENT` checks.
- No UI entry in runtime menus; all surfaces are editor-side (ToolMenus/bridge/MCP).
- Keep graph/spec prompts and DevTools jobs in editor-only contexts; no runtime asset loads via this module.
- Config defaults (Config/DefaultBPGen.ini): loopback-only bridge, safe mode on, audit logs on, remote bridge off by default. Do not flip these for packaged builds.
