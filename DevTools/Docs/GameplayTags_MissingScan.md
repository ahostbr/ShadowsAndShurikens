# Missing Gameplay Tags Scanner

Location: DevTools/python/missing_tags_scan.py

## What it does
- Scans Plugins/** and BEP_EXPORTS/** for tags with roots SOTS./SAS./Input.*.
- Builds add-only patch (+GameplayTagList lines) for missing tags relative to Config/DefaultGameplayTags.ini.
- Emits worklog and patch artifacts under Plugins/SOTS_TagManager/Docs/Worklogs and Docs/TagAudits.
- Optional: appends missing tags to the bottom of Config/DefaultGameplayTags.ini with a comment marker.

## Usage
- Scan only (no file mutation):
  - E:/SAS/ShadowsAndShurikens/.venv_mcp/Scripts/python.exe DevTools/python/missing_tags_scan.py
- Scan and append missing tags to DefaultGameplayTags.ini (bottom of file):
  - E:/SAS/ShadowsAndShurikens/.venv_mcp/Scripts/python.exe DevTools/python/missing_tags_scan.py --apply-default

## Output artifacts
- Patch: Plugins/SOTS_TagManager/Docs/TagAudits/MissingGameplayTags_Patch_<timestamp>.ini
- Worklog: Plugins/SOTS_TagManager/Docs/Worklogs/BuddyWorklog_<timestamp>_MissingTagsAudit.md

## Notes
- Append mode writes a header before the added tags:
  - ; === Missing GameplayTags Auto-Append ===
  - ; Generated: <timestamp>
  - ; Source: <patch filename>
- All emitted lines use +GameplayTagList.
- Skips path-like suffixes (txt/json/csv/md/ini/cpp/cc/cxx/uasset/umap) to avoid false positives.
- Scope is text scan only; no build/editor verification.
