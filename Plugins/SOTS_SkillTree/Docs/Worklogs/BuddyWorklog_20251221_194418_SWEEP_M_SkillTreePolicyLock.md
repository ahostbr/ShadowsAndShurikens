# BuddyWorklog 20251221_194418 SWEEP M SkillTreePolicyLock

goal
- Capture the locked SkillTree registration/unlock expectations, anchor the policies, and keep the plugin free of stale build artefacts.

what changed
- Documented the registration timing, shared point pool, hybrid tags/ids, and unlock broadcast surface in SOTS_SkillTree_RegistrationAndUnlockPolicy.md.
- Added the context anchor for this policy pass and recorded the cleanup of per-plugin Binaries/Intermediate folders.
- Logged this pass per the Buddy requirements and ensured the workspace does not retain stale SOTS_SkillTree build outputs.

files changed
- Plugins/SOTS_SkillTree/Docs/SOTS_SkillTree_RegistrationAndUnlockPolicy.md (policy reference)
- Plugins/SOTS_SkillTree/Docs/Anchor/Buddy_ContextAnchor_20251221_194418_SkillTreePolicyLock.md
- Plugins/SOTS_SkillTree/Docs/Worklogs/BuddyWorklog_20251221_194418_SWEEP_M_SkillTreePolicyLock.md
- Plugins/SOTS_SkillTree/Binaries/ (deleted)
- Plugins/SOTS_SkillTree/Intermediate/ (deleted)

notes + risks/unknowns
- Unlock broadcasts and FX/Stats consumers have not been exercised; the behavior is inferred from existing code paths.
- Removing Binaries/Intermediate assumes no additional generated assets are required for this documentation pass.

verification status
- Not verified (no builds or runtime checks per guardrail).

follow-ups / next steps
- Have Ryan verify OnSkillNodeUnlocked/OnSkillTreeStateChanged fire during mission/safehouse flows so downstream hooks can react without polling.
- Confirm SOTS_SkillTree_RegistrationAndUnlockPolicy.md remains accurate if any registration or unlock entrypoints move.
- Rebuild the plugin if the deleted directories are reported as missing during future work.
