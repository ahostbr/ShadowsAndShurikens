# BuddyWorklog 20251221_200854 SWEEP N GSMPolicyLock

goal
- Capture the LightProbe + player stealth + GSM + dragon meter data flow so the locked responsibilities remain traceable.

what changed
- Authored SOTS_GSM_Producers_Tiers_DragonMeter.md with the complete producer -> GSM -> dragon path, including tier math and timer cadence.
- Added the required context anchor and deleted the plugin Binaries/Intermediate folders to keep the workspace clean.
- Logged the policy pass per the Buddy workflow and confirmed the doc ties back to the exact subsystem/component entrypoints.

files changed
- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_Producers_Tiers_DragonMeter.md
- Plugins/SOTS_GlobalStealthManager/Docs/Anchor/Buddy_ContextAnchor_20251221_200854_GSMPolicyLock.md
- Plugins/SOTS_GlobalStealthManager/Docs/Worklogs/BuddyWorklog_20251221_200854_SWEEP_N_GSMPolicyLock.md
- Plugins/SOTS_GlobalStealthManager/Binaries/ (deleted)
- Plugins/SOTS_GlobalStealthManager/Intermediate/ (deleted)

notes + risks/unknowns
- Real-time firing of OnStealthStateChanged/OnStealthTierChanged during a mission run remains unverified per the 'no builds/tests' guard.
- Removing the Binaries/Intermediate folders assumes no future work needs those artifacts before a rebuild.

verification status
- Not verified (per guardrail; no builds or runtime checks performed).

follow-ups / next steps
- Have Ryan verify the dragon meter receives the canonical FSOTS_PlayerStealthState updates that the doc describes.
- Revisit the doc if new producers (e.g., weather or dragon-specific inputs) start writing to the PlayerStealthComponent.
- Rebuild the plugin if deleting the Binaries/Intermediate directories causes missing-file warnings later.