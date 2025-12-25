Send2SOTS
# Buddy Worklog — 2025-12-25 13:00 — BPGen Bridge Blueprint Ops

## Goal
Add BPGen bridge support for the remaining VibeUE `manage_blueprint` actions (blueprint-level info/property/reparent/custom events/event graph summary), so the unified MCP layer can complete the stubs.

## What Changed
- Added a new Blueprint-ops module in the bridge implementing:
  - `bp_blueprint_get_info`
  - `bp_blueprint_get_property`
  - `bp_blueprint_set_property` (mutating)
  - `bp_blueprint_reparent` (mutating)
  - `bp_blueprint_list_custom_events`
  - `bp_blueprint_summarize_event_graph`
- Wired these actions into `SOTS_BPGenBridgeServer` request dispatch.
- Marked `bp_blueprint_set_property` and `bp_blueprint_reparent` as **dangerous** so safe-mode blocks them unless `dangerous_ok=true`.
- Added `BlueprintGraph` module dependency (needed for `UK2Node_CustomEvent`).
- Deleted plugin `Binaries/` and `Intermediate/` once to avoid stale build artifacts (no build triggered).

## Files Changed
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeBlueprintOps.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeBlueprintOps.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/SOTS_BPGen_Bridge.Build.cs

## Notes / Risks / Unknowns
- `GetInfo` includes a **supported-only + capped** dump of class defaults (simple scalar/text/name types; max 256 entries).
- `SetProperty` uses `FProperty::ImportText_Direct` on the Blueprint CDO; callers must supply correct ImportText (Python wrapper uses existing `_ue_import_text`).
- `Reparent` resolves classes by full object path or by scanning loaded `UClass` objects; this is best-effort and may not find unloaded classes.
- `SummarizeEventGraph` uses the first `UbergraphPages[0]` as the Event Graph; if projects have multiple ubergraphs, this is simplified.

## Verification Status
UNVERIFIED
- No compile/build run.
- No in-editor validation.

## Follow-ups / Next Steps (Ryan)
- Compile the plugin and confirm `BlueprintGraph` dependency resolves.
- From MCP, call each `bp_blueprint_*` action against a known BP and confirm payload shapes.
- Confirm safe-mode behavior blocks `set_property`/`reparent` unless `dangerous_ok=true`.
