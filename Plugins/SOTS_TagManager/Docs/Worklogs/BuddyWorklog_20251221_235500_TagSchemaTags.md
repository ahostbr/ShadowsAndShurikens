# Buddy Worklog 20251221_235500 TagSchema Tags

## Goal
- Expand the global gameplay tag list so the fresh GSM handles, Input buffer layers, Dragon control states, and stealth tiers requested by the team exist in `DefaultGameplayTags.ini`.

## What changed
- Added GSM.Handle.Config/Modifier alongside the BodyDrag block so the GSM handle states have canonical tags.
- Added `Input.Buffer.Channel.Vanish` to the auto-generated input buffer section so the vanish channel becomes part of the gameplay tag table.
- Added `SAS.Dragon.Control.*` tags near the SAS.TagSchema block and inserted the requested stealth-tiers (Alerted/Suspicious/Visible) under the GlobalStealthManager section.
- Left the rest of the locked schema entries untouched; this is purely an additive change in the master tag list.

## Files changed
- Config/DefaultGameplayTags.ini

## Notes + Risks/Unknowns
- `DefaultGameplayTags.ini` is authoritative for the project, so any further automated tag sync may reorganize the file—rerun the Sync workflow if tags shift unexpectedly.
- No validation run was performed; the tag table now contains more entries but their runtime registration is unverified.

## Verification status
- Not verified (per instructions: no build/run).

## Follow-ups / Next steps
- Have Ryan or QA confirm the new tags surface in the TagManager and that telemetry/UI systems referenced by these tags behave as expected.
