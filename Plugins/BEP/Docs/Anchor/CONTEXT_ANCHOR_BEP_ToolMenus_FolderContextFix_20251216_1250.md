[CONTEXT_ANCHOR]
ID: 20251216_1250 | Plugin: BEP | Pass/Topic: ToolMenus_FolderContextFix | Owner: Buddy
Scope: Fix Content Browser folder right-click ToolMenus context to UE 5.7.1 type for BEP export entry.

DONE
- Folder ToolMenus entry now uses UContentBrowserFolderContext with GetSelectedPackagePaths() before calling ExecuteExportFolderWithBEP.
- ContentBrowserMenuContexts.h included (already present) to support the correct context type.

VERIFIED
- UNVERIFIED (no build/run performed in this pass).

UNVERIFIED / ASSUMPTIONS
- Export panel prefill still behaves as previously implemented; not re-validated.

FILES TOUCHED
- Plugins/BEP/Source/BEP/Private/BEP.cpp
- Plugins/BEP/Docs/BuddyWorklog_20251216_125024_BEP_FolderToolMenus_ContextFix.md
- Plugins/BEP/Docs/Anchor/CONTEXT_ANCHOR_BEP_ToolMenus_FolderContextFix_20251216_1250.md

NEXT (Ryan)
- Rebuild BEP and confirm folder right-click shows "Export Folder with BEP" and opens/prefills exporter tab.
- Run a quick export from a folder to ensure menu action still drives ExecuteExportFolderWithBEP.

ROLLBACK
- Revert Plugins/BEP/Source/BEP/Private/BEP.cpp to previous commit/state to restore prior folder menu context behavior.
