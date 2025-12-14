# Buddy Worklog — SOTS SkillTree Gating Helpers (2025-12-13 15:28:00)

## 1) Summary
Added stable read-only gating queries on the SkillTree subsystem plus Blueprint library wrappers so other systems can ask about unlocked skill nodes via gameplay tags.

## 2) Context
Goal: expose a single blessed API for unlock checks (no ad-hoc flags), without changing save formats or gameplay flow.

## 3) Changes Made
- Added gameplay-tag-based gating queries on `USOTS_SkillTreeSubsystem` (single, all, any) using authored `SkillTag` on node definitions.
- Exposed `USOTS_SkillTreeLibrary` wrappers with WorldContext to reach the subsystem safely and mirror empty-list semantics.

## 4) Blessed API Surface
Subsystem:
- `bool IsNodeUnlocked(FGameplayTag NodeTag) const`
- `bool AreAllNodesUnlocked(const TArray<FGameplayTag>& NodeTags) const`
- `bool IsAnyNodeUnlocked(const TArray<FGameplayTag>& NodeTags) const`

Blueprint library:
- `SkillTree_IsNodeUnlocked(UObject* WorldContextObject, FGameplayTag NodeTag)`
- `SkillTree_AreAllNodesUnlocked(UObject* WorldContextObject, const TArray<FGameplayTag>& NodeTags)`
- `SkillTree_IsAnyNodeUnlocked(UObject* WorldContextObject, const TArray<FGameplayTag>& NodeTags)`

## 5) Integration Seam
- Uses authored `SkillTag` on `FSOTS_SkillNodeDefinition` and current runtime unlock state. Null/missing subsystem returns safe defaults.

## 6) Testing
- Not run (API-only change).

## 7) Risks / Considerations
- O(N) over provided tags and unlocked nodes; acceptable for typical tree sizes. Invalid/empty tags handled per rules.

## 8) Cleanup Confirmation
- No plugin binaries cleanup required (game-module change only). Removal of `Plugins/SOTS_SkillTree/Binaries` and `Plugins/SOTS_SkillTree/Intermediate` performed post-change.
