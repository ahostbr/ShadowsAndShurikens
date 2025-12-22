[CONTEXT_ANCHOR]
ID: 20251221_1918 | Plugin: SOTS_FX_Plugin | Pass/Topic: FXOverflowLogging | Owner: Buddy
Scope: Consolidate FX logs under LogSOTS_FX, document the one-shot contract, and make overflow rejection logging explicit.

DONE
- Replaced all `LogTemp` usage inside `USOTS_FXManagerSubsystem` with the new `LogSOTS_FX` category and defined the category in the subsystem headers/CPP.
- Added `LogPoolOverflow` plus per-cue/total pool cap logging in both `AcquireNiagaraComponent` and `AcquireAudioComponent` so `RejectNew` and max-pooled rejections are visible to dev builds.
- Captured the one-shot entry points/pipeline/failure contract in `SOTS_FX_OneShotContract.md` and recorded the worklog entry for these changes.

VERIFIED
- None (no builds or runtime checks were run as part of this change).

UNVERIFIED / ASSUMPTIONS
- Overflow logs and request-failure warnings should appear once someone hits the cap in a non-shipping build; this is assumed until manually exercised.

FILES TOUCHED
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Docs/SOTS_FX_OneShotContract.md
- Plugins/SOTS_FX_Plugin/Docs/Worklogs/BuddyWorklog_20251221_191829_FXOverflowLogging.md

NEXT (Ryan)
1. Trigger FX requests in a dev build until `MaxActivePerCue`/`MaxPooled*` caps are hit to confirm `LogSOTS_FX` warns appear with the expected tag and reason.
2. Review `SOTS_FX_OneShotContract.md` and confirm it captures the guarantees that Blueprint callers rely on.

ROLLBACK
- Revert the files listed above (or `git checkout --` them) to restore the previous logging/pooling contract.
