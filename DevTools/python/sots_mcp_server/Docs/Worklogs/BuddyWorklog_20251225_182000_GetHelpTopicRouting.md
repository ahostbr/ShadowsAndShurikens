Send2SOTS
# Buddy Worklog — 2025-12-25 18:20:00 — get_help topic routing

## Goal
Complete VibeUE parity item (13) `get_help` by implementing topic-based routing instead of returning only the `sots_help` index.

## What Changed
- Implemented real topic routing for `get_help(topic=...)` in the unified MCP server.
- `get_help` now loads markdown content from the VibeUE resource bundle:
  - preferred: `Plugins/VibeUE/Content/Python/resources/topics/<topic>.md`
  - fallback: `topics/topics.md` (topic index)
  - fallback: `help.md` (full reference)
- Normalizes topic names (`_` → `-`) and supports `topic="help"|"index"`.
- Preserves prior behavior by also returning the unified `sots_help` index alongside the topic content.
- Updated parity matrix entry for item (13) from Partial → Done.

## Files Changed
- DevTools/python/sots_mcp_server/server.py
- Plugins/SOTS_BlueprintGen/Docs/VibeUE_Upstream_ParityMatrix.md

## Notes / Risks / Unknowns
- Topic content is sourced from the VibeUE plugin content in-repo; if that directory is missing, `get_help` returns a structured error.
- No attempt was made to invent or modify help content; this change only routes to existing markdown files.

## Verification
UNVERIFIED
- No MCP client run and no Unreal/editor validation performed.

## Next (Ryan)
- Call `get_help()` and `get_help(topic="topics")` from an MCP client and confirm expected markdown is returned.
- Spot check one topic (e.g., `node-tools`) for correct file resolution and payload shape.

## Rollback
- Revert the files listed above.
