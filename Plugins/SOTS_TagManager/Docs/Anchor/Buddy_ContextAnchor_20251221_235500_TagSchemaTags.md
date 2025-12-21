[CONTEXT_ANCHOR]
ID: 20251221_235500 | Plugin: SOTS_TagManager | Pass/Topic: TagSchema Additions | Owner: Buddy
Scope: Ensure the shared gameplay tag table contains the requested GSM handles, input buffer channels, Dragon control states, and stealth tier buckets.

DONE
- Added `GSM.Handle.Config` and `GSM.Handle.Modifier` entries near the BodyDrag section so GSM handle metadata can surface as identity tags.
- Extended the automatic input buffer block with `Input.Buffer.Channel.Vanish` so the vanish queue is discoverable by the buffer manager.
- Added the `SAS.Dragon.Control.*` control-state tags and the missing `SAS.Stealth.Tier.Alerted/Suspicious/Visible` tags in the locked schema region.

VERIFIED
- None (no editor or build pass run).

UNVERIFIED / ASSUMPTIONS
- The new tags are syntactically correct and loadable, but their runtime registration has not been confirmed.

FILES TOUCHED
- Config/DefaultGameplayTags.ini

NEXT (Ryan)
- Confirm the new tags appear under `SOTS_TagManager` after startup or tag sync, and that anything referencing them (GSM handles, stealth tiers, UI buffer layers) observes the expected state names.

ROLLBACK
- Revert Config/DefaultGameplayTags.ini if the new tags need to be removed.
