[CONTEXT_ANCHOR]
ID: 20251223_042000 | Plugin: SOTS_TagManager | Pass/Topic: TagSchemaTest | Owner: Buddy
Scope: Ensure the gameplay tag schema supports the BPGen data asset builder test case (`Test.Test`).

DONE
- Added a redirect from `Test.Test` to the new `SAS.Test.Test` entry in `Config/DefaultGameplayTags.ini` so the BPGen payload can stay literal while the authoritative tag uses the SAS namespace.
- Appended the `SAS.Test.Test` entry to the gameplay tag list so the tag manager can resolve and cache the cue tag during the editor run.

VERIFIED
- BPGen’s `create_data_asset` call with `CueTag=Test.Test` succeeded while the editor (and the BSP bridge) was running after the config refresh.

UNVERIFIED / ASSUMPTIONS
- No other systems currently consume `SAS.Test.Test`, so downstream behavior is unverified.

FILES TOUCHED
- Config/DefaultGameplayTags.ini

NEXT (Ryan)
- If more tags should be available for debugging data asset creation, follow the existing add-only workflow and keep them under `SAS.*`.

ROLLBACK
- Remove the new redirect/tag entries from `Config/DefaultGameplayTags.ini` to revert the test alias.
