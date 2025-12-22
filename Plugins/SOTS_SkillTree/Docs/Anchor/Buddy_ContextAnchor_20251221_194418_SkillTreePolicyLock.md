[CONTEXT_ANCHOR]
ID: 20251221_194418 | Plugin: SOTS_SkillTree | Pass/Topic: SkillTreePolicyLock | Owner: Buddy
Scope: Document the registration/unlock guarantee surface that protects SOTS_SkillTree interactions.

DONE
- Captured the registration, point-pool, hybrid identity, and unlock-broadcast expectations in SOTS_SkillTree_RegistrationAndUnlockPolicy.md and committed the referenced entrypoints.
- Added the mandated context anchor for this policy pass and documented the cleanup of per-plugin build artifacts.
- Logged the pass via BuddyWorklog_20251221_194418_SWEEP_M_SkillTreePolicyLock.md while keeping Binaries and Intermediate out of the repo.

VERIFIED
- None (per "no builds/tests" guard).

UNVERIFIED / ASSUMPTIONS
- OnSkillNodeUnlocked's runtime firing/FX consumption has not been validated; behavior is assumed to match the documented flow.
- Deleting Binaries/Intermediate assumes no further generated assets are required for this task.

FILES TOUCHED
- Plugins/SOTS_SkillTree/Docs/Anchor/Buddy_ContextAnchor_20251221_194418_SkillTreePolicyLock.md
- Plugins/SOTS_SkillTree/Docs/Worklogs/BuddyWorklog_20251221_194418_SWEEP_M_SkillTreePolicyLock.md
- Plugins/SOTS_SkillTree/Binaries/ (deleted)
- Plugins/SOTS_SkillTree/Intermediate/ (deleted)

NEXT (Ryan)
- Verify OnSkillNodeUnlocked/OnSkillTreeStateChanged fire as expected by instrumenting a mission-safehouse flow so UI/FX/Stats can react without polling.
- Re-review SOTS_SkillTree_RegistrationAndUnlockPolicy.md after any refactor that might move registration or unlock entrypoints; update file references if they change.
- Ensure deleting the Binaries and Intermediate folders hasn’t removed any required artifacts; rebuild the plugin if a missing file is reported.

ROLLBACK
- git checkout -- Plugins/SOTS_SkillTree/Docs/Anchor/Buddy_ContextAnchor_20251221_194418_SkillTreePolicyLock.md
- git checkout -- Plugins/SOTS_SkillTree/Docs/Worklogs/BuddyWorklog_20251221_194418_SWEEP_M_SkillTreePolicyLock.md
- Recreate Plugins/SOTS_SkillTree/Binaries/ and Plugins/SOTS_SkillTree/Intermediate/ from the latest build output if necessary.
