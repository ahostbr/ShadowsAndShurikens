# Buddy Worklog — 2025-12-16 12:50:24 — BEP Folder ToolMenus Context Fix

## Goal
Fix the Content Browser folder right-click ToolMenus context to the correct UE 5.7.1 type so BEP export menu compiles and works for folders.

## What changed
- In BEP.cpp, folder menu lambda now uses `UContentBrowserFolderContext` with `GetSelectedPackagePaths()` (replacing the invalid `UContentBrowserMenuContext_Folder` + `SelectedPaths`).
- Includes already covered `ContentBrowserMenuContexts.h`; no other files needed changes.

## Files touched
- Plugins/BEP/Source/BEP/Private/BEP.cpp

## Verification
- UNVERIFIED (no build/run in this pass).

## Cleanup
- Deleted Plugins/BEP/Binaries and Plugins/BEP/Intermediate per policy.
