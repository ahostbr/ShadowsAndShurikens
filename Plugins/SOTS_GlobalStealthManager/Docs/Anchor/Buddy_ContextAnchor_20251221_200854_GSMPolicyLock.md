[CONTEXT_ANCHOR]
ID: 20251221_200854 | Plugin: SOTS_GlobalStealthManager | Pass/Topic: GSMPolicyLock | Owner: Buddy
Scope: Capture the LightProbe/PlayerStealth/GSM/Dragon meter data flow expected by the locked responsibility list.

DONE
- Added the policy doc describing the producer inputs, tier computation, alertness cadence, and dragon meter contract.
- Logged the pass via the GSM policy worklog and documented the cleanup of the plugin's Binaries/Intermediate folders.
- Confirmed all data references tie to the subsystem/dragon implementation so the doc links back to concrete entrypoints.

VERIFIED
- None (no builds or runtime verification per the guardrail).

UNVERIFIED / ASSUMPTIONS
- OnStealthStateChanged/OnStealthTierChanged firing in the actual mission flow is assumed to match the documented behavior.

FILES TOUCHED
- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_Producers_Tiers_DragonMeter.md
- Plugins/SOTS_GlobalStealthManager/Docs/Anchor/Buddy_ContextAnchor_20251221_200854_GSMPolicyLock.md
- Plugins/SOTS_GlobalStealthManager/Docs/Worklogs/BuddyWorklog_20251221_200854_SWEEP_N_GSMPolicyLock.md
- Plugins/SOTS_GlobalStealthManager/Binaries/ (deleted)
- Plugins/SOTS_GlobalStealthManager/Intermediate/ (deleted)

NEXT (Ryan)
- Confirm GSM broadcasts reach the dragon meter and that the light/tier/detection values match the mission instrumentation.
- Re-open the doc if the LightProbe cadence changes or new producers start writing to the PlayerStealthComponent.
- Rebuild the plugin if any missing binaries/intermediate folders are required for future work.

ROLLBACK
- git checkout -- Plugins/SOTS_GlobalStealthManager/Docs/SOTS_GSM_Producers_Tiers_DragonMeter.md
- git checkout -- Plugins/SOTS_GlobalStealthManager/Docs/Anchor/Buddy_ContextAnchor_20251221_200854_GSMPolicyLock.md
- git checkout -- Plugins/SOTS_GlobalStealthManager/Docs/Worklogs/BuddyWorklog_20251221_200854_SWEEP_N_GSMPolicyLock.md
- Restore Plugins/SOTS_GlobalStealthManager/Binaries/ and Plugins/SOTS_GlobalStealthManager/Intermediate/ from the last build output if needed.