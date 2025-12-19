# Buddy Worklog — 2025-12-16 12:08:48 — BEP context menu

## Goal
Restore BEP right-click Content Browser menu entry to match BlueprintExporter behavior so users can export folders via BEP.

## What changed
- Updated BEP asset menu extender to hook into the Content Browser asset view section `GetAssetActions`, mirroring BlueprintExporter so the entry shows up on asset/folder right-clicks.
- Context menu action now opens the BEP Exporter tab, prefills the selected folder path and a suggested output root, and leaves the user to manually click Run.
- Forward-declared `FAssetData` in the module header and included `AssetRegistry/AssetData.h` in the cpp to resolve missing include during build.
- Folder context-menu hook switched to `PathContextMenu` so the entry appears when right-clicking folders in the Content Browser.
- Added ToolMenus hook on `ContentBrowser.FolderContextMenu` (section `PathViewFolderOptions`) so the folder right-click menu always shows the BEP export entry; still prefills and opens the exporter tab.

## Files touched
- Plugins/BEP/Source/BEP/Private/BEP.cpp
- Plugins/BEP/Source/BEP/Public/BEP.h
- Plugins/BEP/Source/BEP/Private/Widgets/SBEPExportPanel.h
- Plugins/BEP/Source/BEP/Private/Widgets/SBEPExportPanel.cpp

## Verification
- Not run (UI/menu change only).

## Cleanup
- Deleted Plugins/BEP/Binaries and Plugins/BEP/Intermediate per plugin edit policy.

## Follow-ups
- Rebuild BEP and confirm "Export Folder with BEP" appears when right-clicking assets and asset-view folders.
- Verify the tab opens and fields prefill with the first selected folder before running manually.
