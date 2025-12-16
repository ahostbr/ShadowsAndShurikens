# Buddy Worklog — 2025-12-16 12:38 — BEP ToolMenus FolderContext fix

## Goal
Fix BEP folder right-click ToolMenus context to match UE 5.7.1 API so the export entry works on folders.

## What changed
- Updated Content Browser folder ToolMenu entry in BEP.cpp to use `UContentBrowserFolderContext` and `GetSelectedPackagePaths()` instead of the invalid `UContentBrowserMenuContext_Folder/SelectedPaths`.
- Includes for ContentBrowser menu contexts were already present; no other files changed.

## Files touched
- Plugins/BEP/Source/BEP/Private/BEP.cpp

## Menu anchors
- Window menu: LevelEditor.MainMenu.Window (unchanged)
- Folder menu: ContentBrowser.FolderContextMenu, section PathViewFolderOptions (BEP entry lives here)

## Verification
- Not run (compile-only change). Build expected to succeed once regenerated.

## Cleanup
- Deleted Plugins/BEP/Binaries and Plugins/BEP/Intermediate per plugin edit policy.
