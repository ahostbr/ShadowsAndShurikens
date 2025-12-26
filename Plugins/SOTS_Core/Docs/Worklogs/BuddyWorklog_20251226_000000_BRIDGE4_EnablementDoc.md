# Buddy Worklog — SOTS_Core — BRIDGE4 Enablement Doc

Date: 2025-12-26
Owner: Buddy
Plugin: SOTS_Core
Pass/Topic: BRIDGE4 (Bridge enablement order consolidation)

## Goal
Create the BRIDGE4 required consolidated documentation for safely enabling SOTS_Core bridges (BRIDGE1–BRIDGE3), with correct command names and OFF-by-default posture.

## What changed
- Added a consolidated enablement-order doc in SOTS_Core that:
  - Lists the participating settings sections/keys for Core + bridged plugins.
  - Lists authoritative SOTS_Core diagnostics console command names.
  - Provides a step-by-step enablement sequence: Core dispatch → Input bridge → ProfileShared bridge → UI bridge.
  - Includes rollback steps and notes on what logs/signals to expect.

## Files changed
- Plugins/SOTS_Core/Docs/SOTS_Core_Bridge_Enablement_Order.md
- Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_000000_BRIDGE4_EnablementDoc.md

## Notes / Risks / Unknowns
- UNVERIFIED: No build/editor run performed (per Buddy rules). Command names and settings names were sourced from code inspection prior to this doc.
- The expected verbose log strings can drift over time; the doc treats them as “what to look for” rather than a strict contract.

## Verification status
- VERIFIED: N/A (no runtime/editor verification performed).
- UNVERIFIED: Entire flow; requires Ryan to validate in-editor.

## Next steps (Ryan)
- Run `SOTS.Core.DumpSettings` and `SOTS.Core.Health` in-editor to confirm diagnostics are available.
- Flip `bEnableLifecycleListenerDispatch=true`, then enable bridges one at a time per doc; confirm no unintended behavior changes.

## Rollback
- Revert the doc files listed above.
