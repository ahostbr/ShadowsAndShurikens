# BPGen SPINE K - Batch, Sessions, Headless, Perf (2025-12-19)

## Summary
- Added batch/session actions to the bridge with per-step timing and summaries.
- Added cache priming/limits controls plus recent-request ring buffer.
- Added timing fields and structured request logging for observability.

## Bridge changes
- New actions: `batch`, `begin_session`, `end_session`, `session_batch`, `prime_cache`, `clear_cache`, `set_limits`, `get_recent_requests`.
- Added timing fields to responses: `server.request_ms`, `server.dispatch_ms`, `server.serialize_ms`.
- Added per-request logs (LogSOTS_BPGenBridge) and verbose per-step batch logs.
- Added limits: `max_discovery_results`, `max_pin_harvest_nodes`, `max_autofix_steps` with clamping.

## BPGen changes
- Added spawner cache priming helper in `FSOTS_BPGenSpawnerRegistry::PrimeCache`.

## Files touched
- `Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGenBridgeServer.h`
- `Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp`
- `Plugins/SOTS_BPGen_Bridge/Docs/BPGen_Bridge_Protocol.md`
- `Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BPGenSpawnerRegistry.h`
- `Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenSpawnerRegistry.cpp`

## Notes
- Headless runner and batch submission utilities are implemented in DevTools (see DevTools worklog).
- No build/run performed (code-only verification).
