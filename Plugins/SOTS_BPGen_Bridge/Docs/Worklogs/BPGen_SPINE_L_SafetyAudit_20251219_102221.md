# BPGen SPINE L - Safety + Audit (2025-12-19)

## Summary
- Added safety/guardrail controls to the BPGen bridge (safe mode, allow/deny lists, dangerous op gating).
- Added `server_info`, `health`, and `emergency_stop` actions.
- Added audit log output per request with sanitized params.

## Bridge changes
- Local-only enforcement (`bAllowNonLoopbackBind`).
- Dangerous ops require `params.dangerous_ok`; safe mode blocks dangerous ops.
- Allow/deny action lists (`AllowedActions`, `DeniedActions`).
- Per-minute rate limit support (`MaxRequestsPerMinute`).
- Audit logs: `Plugins/SOTS_BPGen_Bridge/Saved/BPGenAudit/YYYYMMDD/<request_id>_<action>.json`.

## Files touched
- `Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGenBridgeServer.h`
- `Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp`
- `Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md`
- `Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Safety.md`

## Notes
- Change summaries are attached to results where available and included in audit logs.
- No build/run performed (code-only verification).
