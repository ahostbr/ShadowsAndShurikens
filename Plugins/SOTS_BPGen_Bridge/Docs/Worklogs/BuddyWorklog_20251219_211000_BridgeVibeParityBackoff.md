# Buddy Worklog — 2025-12-19 21:10:00

## Goal
Tighten 1:1 parity with VibeUE MCP runnable backoff and recv error handling.

## What changed
- Accept loop now sleeps 0.1s when no pending connections and on Accept failure (matches VibeUE pacing).
- Recv loop now treats SE_EWOULDBLOCK and SE_EINTR as transient (log Verbose, sleep 10ms, continue) before breaking on other errors.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

## Notes / risks / unknowns
- Behavior still unverified; pacing differences could affect throughput but should reduce busy spin.

## Verification
- Not run.

## Follow-ups / next steps
- Rebuild SOTS_BPGen_Bridge, restart bridge, run bpgen_ping with Verbose logs to confirm framing/output parity.
