# Buddy Worklog — BEP Node JSON Export (SPINE2)

## Goal
Make the node JSON exporter schema-complete, deterministic, preset-driven, and add clipboard flow plus persistent settings.

## Changes
- Added per-project settings UObject: [Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExportSettings.h](Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExportSettings.h), [Private/BEP_NodeJsonExportSettings.cpp](Plugins/BEP/Source/BEP/Private/BEP_NodeJsonExportSettings.cpp). Stores default preset, depth limits, compact/pretty toggles, pin/edge toggles, class dict/legend flags, and title/comment inclusion.
- Expanded exporter surface: [Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExport.h](Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExport.h) adds presets (SuperCompact, Compact, AI Minimal, AI Audit), new options (class_dict, pin decl/full/unconnected), and returns node/edge counts.
- Reworked exporter implementation: [Plugins/BEP/Source/BEP/Private/BEP_NodeJsonExport.cpp](Plugins/BEP/Source/BEP/Private/BEP_NodeJsonExport.cpp)
  - Emits schema_version=1, compact vs verbose key sets, class_dict mapping codes->full class path, optional legend, deterministic sorting of nodes/pins/edges, edge de-dupe, BFS expansion by exec/data depth, stable IDs from NodeGuid.
  - Pin type payload matches case-study style: cg/sc/sco/ct/rf/cs with optional default_raw, flags, and audit-only full/unused pin buckets.
- ToolMenus wiring: [Plugins/BEP/Source/BEP/Private/BEP.cpp](Plugins/BEP/Source/BEP/Private/BEP.cpp)
  - Uses settings-driven options (BuildOptionsFromSettings).
  - Updated “BEP: Export Node JSON (Selection)” to log/toast counts and preset, and save under `BEP_EXPORTS/NodeJSON/<Blueprint>/<stem>.json`.
  - Added “BEP: Copy Node JSON (Selection)” to copy JSON to clipboard with toast/log and counts.
- Build.cs: added ApplicationCore dependency for clipboard support.

## Schema (compact keys)
- Root: sv (schema_version), e (engine), a (asset), g (graph), cd (class_dict), lg (legend), n (nodes[]), x (edges[])
- Node: i (id), c (class code), nm (name), cm (comment), p (pos:{x,y}), pd (pin_decls[]), p (full pins, audit), pu (unconnected pins, audit)
- Pin decl: n (name), d (dir 0/1), k (kind 0 exec/1 data), t (type{cg,sc,sco,ct,rf,cs}), dr (default_raw), fl (flags, optional)
- Edge: k (kind 0 exec/1 data), fn/fp/tn/tp (from/to node id + pin)

Verbose keys mirror these with readable names (schema_version, engine, asset, graph, class_dict, nodes, edges, etc.).

## Presets (enforced in ApplyPreset)
- SuperCompact: compact, pretty off, class_dict on, pin_decls on, pos on, titles/comments off, no full/unconnected pins.
- Compact: compact, pretty off, class_dict on, pin_decls on, titles on, comments off.
- AI Minimal: verbose, pretty on, class_dict off, pin_decls on, titles on, comments off.
- AI Audit: verbose, pretty on, class_dict off, pin_decls on, full pins on, unconnected pins on, titles/comments on.

## Verification
- No build/run executed per instructions.
- Menu entries exist under LevelEditor → Window → SOTS Tools. Actions toast + UE_LOG with `[BEP][NodeJSON]` prefix and include node/edge counts.

## Cleanup
- Removed Binaries/ and Intermediate/ under Plugins/BEP after edits.
