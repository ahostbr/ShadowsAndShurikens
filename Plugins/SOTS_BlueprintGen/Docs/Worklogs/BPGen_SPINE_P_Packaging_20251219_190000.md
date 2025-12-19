# BPGen_SPINE_P — Packaging + Docs + Templates

## Goal
Lift BPGen to marketplace-grade readiness using VibeUE parity as reference (docs/templates/examples, safety defaults, distribution toggles).

## Changes
- Added packaging/docs set: ModuleBoundaries, ActionReference, ErrorCodes, ResultSchemas, GettingStarted, FAQ, ThirdPartyNotices, Attribution, ReleaseChecklist.
- Added config defaults (DefaultBPGen.ini) with safe loopback-only bridge, safe_mode on, audit logs on, remote bridge off.
- Added redistributable templates (GraphSpecs/Jobs/Replays) and updated Examples README/ExpectedOutcomes.
- Kept examples/templates JSON-only to avoid restricted content.

## Verification
- Not built or run (docs/config/templates only).

## Notes / Risks
- Config settings are declarative; runtime code does not yet consume them—intended for future wiring.
- Templates use `$include` placeholders; consumers must resolve includes before calling MCP.
