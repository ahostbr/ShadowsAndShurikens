# Worklog - SOTS_MMSS resume policy lock (Buddy)

## Goal
- Lock down the expectations around mission-less track state, the "close enough" resume timer, and the multi-layer plan/absence so future sweeps know what the subsystem currently promises.

## What changed
- Added `Docs/SOTS_MMSS_ResumePolicy.md` describing how track/mission identity flows through `RequestMusicByTag/RequestMusicByMissionAndTag`, the default-music fallback when `MissionId == NAME_None`, the snapshot-driven bookkeeping, and the tolerant resume gating that is currently disabled.
- Noted the lack of multi-layer plumbing in the subsystem so this policy doc can refer future work back to the existing single-track state.

## Files changed
- Plugins/SOTS_MMSS/Docs/SOTS_MMSS_ResumePolicy.md
- Plugins/SOTS_MMSS/Docs/BuddyWorklog_20251221_201051_SWEEP_Q_MMSSPolicyLock.md

## Notes / risks / unknowns
- Documentation-only pass; no code changes or builds were performed.

## Verification status
- Unverified (doc-only).

## Cleanup
- Plugins/SOTS_MMSS/Binaries: removed after the sweep.
- Plugins/SOTS_MMSS/Intermediate: removed after the sweep.

## Follow-ups / next steps
- If multi-layer requirements land, update the policy doc with the new layer state machine.
