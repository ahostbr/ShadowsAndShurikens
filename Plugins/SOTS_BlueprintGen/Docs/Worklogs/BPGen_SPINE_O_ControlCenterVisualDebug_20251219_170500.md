# BPGen_SPINE_O — Control Center + Visual Debug

## Goal
Add an in-editor BPGen Control Center, visual debug annotations, node focus, and bridge UI hooks per SPINE_O.

## Changes
- New Control Center tab (`SOTS BPGen Control Center`) with bridge start/stop, recent-requests view, and debug actions (annotate, clear annotations, focus node).
- Debug helper library `USOTS_BPGenDebug` to annotate nodes (error bubble with `[BPGEN]` prefix), clear annotations, and focus a node by NodeId without altering BPGen NodeId comments.
- One True Entry API now supports actions `debug_annotate`, `clear_annotations`, `focus_node` for MCP/bridge parity.
- Bridge server exposes `GetRecentRequestsForUI` for UI consumption; feature flag already lists recent_requests.
- Docs: BPGen_ControlCenter.md; anchor added.

## Files
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.*
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenDebug.*
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BlueprintGen.cpp
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Public/SOTS_BlueprintGen.h
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/SOTS_BlueprintGen.Build.cs
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenAPI.cpp
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Public/SOTS_BPGenBridgeServer.h
- Plugins/SOTS_BPGen_Bridge/Source/SOTS_BPGen_Bridge/Private/SOTS_BPGenBridgeServer.cpp
- Plugins/SOTS_BlueprintGen/Docs/BPGen_ControlCenter.md
- Plugins/SOTS_BlueprintGen/Docs/Anchor/Anchor_BPGen_SPINE_O_ControlCenter.md

## Notes / Risks
- Annotate uses error bubble/tint; NodeId comments remain unchanged to avoid breaking ID lookups.
- Focus uses Blueprint editor jump; best-effort only.
- Bridge start/stop remains auto-start on module load; UI start/stop just forwards.

## Verification
- Not built or run (editor-only changes). Pending manual validation in-editor.
