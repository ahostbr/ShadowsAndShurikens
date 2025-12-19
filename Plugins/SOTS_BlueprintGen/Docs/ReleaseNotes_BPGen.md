# Release Notes — SOTS_BlueprintGen

## 2025-12-19 (SPINE_N)
- Added `USOTS_BPGenAPI` One True Entry (BlueprintFunctionLibrary) with JSON action and batch execution.
- Actions supported: apply/canonicalize graph specs, apply function skeletons, delete/replace nodes/links, create struct/enum assets.
- Apply path gated by env `SOTS_ALLOW_APPLY=1`; envelopes serialize JSON in/out for MCP/bridge parity.
