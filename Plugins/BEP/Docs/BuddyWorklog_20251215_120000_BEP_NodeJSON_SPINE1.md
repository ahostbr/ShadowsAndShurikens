# Buddy Worklog — BEP Node JSON Export (SPINE1)

## Goal
Add a compact "Node JSON export" backend to BEP (selection-based, optional neighbor expansion) with a minimal ToolMenus entry.

## Changes
- Added Node JSON export backend:
  - [Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExport.h](Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExport.h)
  - [Plugins/BEP/Source/BEP/Private/BEP_NodeJsonExport.cpp](Plugins/BEP/Source/BEP/Private/BEP_NodeJsonExport.cpp)
  - Supports presets (AI Minimal / Compact / SuperCompact), optional legend, pin/edge capture, neighbor BFS by exec/data depth, compact keys, and save-to-file under BEP export root.
- Hooked minimal ToolMenus entry under LevelEditor → Window → SOTS Tools:
  - Label: "BEP: Export Node JSON (Selection)" (SuperCompact preset by default)
  - On success: saves to `<Project>/BEP_EXPORTS/NodeJSON/<BlueprintName>/<stem>.json` and toasts/UE_LOG with `[BEP][NodeJSON]` prefix.
  - On failure: toasts/UE_LOG failure reason.
- Reused existing BEP export root helper (`GetDefaultBEPExportRoot`) for save location.

## Files touched
- [Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExport.h](Plugins/BEP/Source/BEP/Public/BEP_NodeJsonExport.h)
- [Plugins/BEP/Source/BEP/Private/BEP_NodeJsonExport.cpp](Plugins/BEP/Source/BEP/Private/BEP_NodeJsonExport.cpp)
- [Plugins/BEP/Source/BEP/Private/BEP.cpp](Plugins/BEP/Source/BEP/Private/BEP.cpp)

## Notes / Verification
- No build/run performed (per instructions).
- Menu entry uses SuperCompact preset; can be adjusted in code if needed.
- Neighbor expansion depths default to 0 (selection only) but options struct supports exec/data depth if wired later.

## Cleanup
- Binaries/Intermediate for BEP removed after edits.
