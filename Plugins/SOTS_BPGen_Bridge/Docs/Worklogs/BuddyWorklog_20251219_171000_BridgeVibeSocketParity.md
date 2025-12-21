# Buddy Worklog — 2025-12-19 17:10:00

## Goal
Align bridge socket handling with VibeUE patterns to improve connection stability and JSON delivery.

## What changed
- Listen socket now sets reuse-addr and non-blocking immediately after creation (matching VibeUE listener setup).
- Accepted client sockets now set NoDelay and 64KB send/receive buffers before switching to blocking mode.

## Files changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp

## Notes / risks / unknowns
- No build/run yet; needs rebuild and bridge restart to take effect.

## Verification
- Not run.

## Follow-ups / next steps
- Rebuild SOTS_BPGen_Bridge, restart the bridge, retry bpgen_ping for valid JSON.
