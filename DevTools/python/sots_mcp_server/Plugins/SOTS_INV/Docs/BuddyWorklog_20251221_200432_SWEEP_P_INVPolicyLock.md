# Worklog ƒ?" SOTS_INV Identity/UI policy lock (Buddy)

## Goal
- Capture the identity rule, provider canonicalization, and UI access policy so the INV subsystem stays in lockstep with SOTS_UI for widget ownership.
- Record that there are no additional direct InvSP UI calls and document the remaining policy seams.

## What changed
- Added `Docs/SOTS_INV_IdentityAndUIAccessPolicy.md` with the canonical ItemId<->ItemTag identity, the bridge/facade provider resolution path, and the router-only UI intent surface.
- Noted that `RequestInvSP_*` is only invoked from the facade helpers (and router implementation/docs), reaffirming SOTS_UI as the widget/input owner.
- Created this worklog to tether the policy narrative to the ongoing sweep.

## Files changed
- Plugins/SOTS_INV/Docs/SOTS_INV_IdentityAndUIAccessPolicy.md
- Plugins/SOTS_INV/Docs/BuddyWorklog_20251221_200432_SWEEP_P_INVPolicyLock.md

## Notes / risks / unknowns
- Documentation-only change; no code touched in this pass.

## Verification status
- Unverified (no build/run; doc-only work).

## Cleanup
- Plugins/SOTS_INV/Binaries: absent/deleted already.
- Plugins/SOTS_INV/Intermediate: absent/deleted already.

## Follow-ups / next steps
- Watch for future router or provider interface changes that might require polishing the policy doc.
