[CONTEXT_ANCHOR]
ID: 20251216_1234 | Plugin: BEP | Pass/Topic: ToolMenus Folder Context Fix | Owner: Buddy
Scope: Fix BEP compile errors by using correct UE5 ToolMenus folder context type.

DONE
- Updated ToolMenus folder right-click handler to use:
  - UContentBrowserFolderContext
  - GetSelectedPackagePaths()
- Target menu: ContentBrowser.FolderContextMenu
- Target section: PathViewFolderOptions
- File change: Plugins/BEP/Source/BEP/Private/BEP.cpp

VERIFIED
- None (Buddy did not run build/editor in this pass).  <-- keep honest

UNVERIFIED / ASSUMPTIONS
- Menu entry appears in folder right-click context menu.
- Action still opens BEP exporter tab and pre-fills selected folder(s).
- No duplicate menu entries (if old extender path also registers).

FILES TOUCHED
- Plugins/BEP/Source/BEP/Private/BEP.cpp

NEXT (Ryan)
- Compile the project (or just BEP) and confirm BEP is no longer “missing/incompatible module”.
- In-editor: right-click a folder tile in Content Browser → BEP entry appears once.
- Trigger action with single + multi-selected folders → exporter opens and paths are correctly passed.
- If duplicates appear, gate old extender registration so only ToolMenus registers.

ROLLBACK
- Revert changes to Plugins/BEP/Source/BEP/Private/BEP.cpp (or git revert the commit if available).
