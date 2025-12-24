# Repo Index Notes (Future You)

## Why tags were polluted
The initial tag scan treated any quoted dotted string as a tag. That picked up file names, versions, and IPs.

## Fix applied (repo_indexer.py)
- Added `_is_likely_gameplay_tag(tag)`:
  - Shape: `^[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z0-9_]+)+$`
  - Reject IPv4-like strings.
  - Reject numeric-only segments.
  - Reject file-extension suffixes (h, cpp, uasset, etc.).
  - Reject overly long tags (>128 chars).
- Added trusted extraction contexts (still validated):
  - `UE_DEFINE_GAMEPLAY_TAG` / `UE_DEFINE_GAMEPLAY_TAG_COMMENT`
  - `FGameplayTag::RequestGameplayTag(...)`
  - `AddNativeGameplayTag(...)`
- Kept generic quoted-dotted fallback only when:
  - `_is_likely_gameplay_tag(tag)` is true, and
  - the root segment is allowlisted.

## Allowlist
- Built from ini-defined tags (roots split at first '.')
- Plus default roots: `SOTS, SAS, Input, Interaction, Player, AI, GSM, MMSS, KEM, Mission, SkillTree, INV, UI, FX`
- Applied only to the generic fallback (not to trusted contexts).

## Cache behavior
- Bumped `CACHE_VERSION` to force a clean rescan (old cached tags were polluted).
- Incremental scanning still works as before.

## Expected results
- `tag_usage.json` should no longer include file names, versions, IPs, or numeric dots.
- `tags_missing_from_ini` should now be meaningful.

## Manual sanity check
```
python DevTools/python/sots_repo_index/run_repo_index.py --project_root E:\SAS\ShadowsAndShurikens --verbose --full
```
Check `Reports/RepoIndex/tag_usage.json` for:
- no "*.h", "127.0.0.1", "5.7", "0.0" entries
- reasonable GameplayTags only
