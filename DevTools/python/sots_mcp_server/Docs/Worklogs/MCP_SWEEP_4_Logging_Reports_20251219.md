## MCP Sweep 4: Logging & Reports (2025-12-19)

- Added JSONL logging helper with redaction to `DevTools/python/logs/sots_mcp_server.jsonl`; request_id per call.
- Reports root standardized to `DevTools/python/reports`; list/read remain read-only.
- Added `sots_last_error` tool to fetch the latest logged error (bounded scan).
- Envelope/logging applied to new report handling; stdout remains unused. No tests run.
