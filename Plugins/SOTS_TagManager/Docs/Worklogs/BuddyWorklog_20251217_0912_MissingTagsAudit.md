# Missing GameplayTags Audit (BRIDGE missing-only)
Timestamp: 20251217_0912
Scan: Plugins/** and BEP_EXPORTS/** for SOTS./SAS./Input.* tags
Stats: Used=375 Existing=487 Missing=47
Patch file: E:\SAS\ShadowsAndShurikens\Plugins\SOTS_TagManager\Docs\TagAudits\MissingGameplayTags_Patch_20251217_0912.ini

Goal
- Capture missing-only gameplay tags (SOTS/SAS/Input roots) as add-only `+GameplayTagList` lines for DefaultGameplayTags.ini.

Changes
- Updated DevTools/python/_tmp_missing_tags_scan.py to drop path-like suffixes (txt/json/csv/md/ini/cpp/etc.) from matches.
- Re-ran the scan; regenerated missing-tags patch (47 entries) and refreshed this worklog.

Files Changed
- DevTools/python/_tmp_missing_tags_scan.py
- Plugins/SOTS_TagManager/Docs/TagAudits/MissingGameplayTags_Patch_20251217_0912.ini
- Plugins/SOTS_TagManager/Docs/Worklogs/BuddyWorklog_20251217_0912_MissingTagsAudit.md

Notes / Risks / Unknowns
- Scan scope limited to Plugins/** and BEP_EXPORTS/**; roots: SOTS., SAS., Input.* only.
- Patch not applied to Config/DefaultGameplayTags.ini (add-only output only).
- No editor/runtime verification; assumes tag usage inferred from text scan is desired.

Verification
- UNVERIFIED: No build/editor run; relied solely on regex scan output.

Cleanup
- DevTools-only change; plugin binaries not touched. No cleanup required.

Follow-ups / Next Steps
- Ryan: apply missing tags to Config/DefaultGameplayTags.ini if approved, then rerun the scan to confirm Missing=0.
