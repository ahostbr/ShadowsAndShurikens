# Worklog - SOTS_TagManager Contract Sweep

- Confirmed `USOTS_GameplayTagManagerSubsystem` already drives the union view (`GetActorTags`), scoped handle bookkeeping, and reactive `OnLooseTagAdded`/`OnLooseTagRemoved` events so tag reads/writes stay centralized.
- Captured the runtime contract (boundary tags, scoped handles, cleanup, events, union view) in `SOTS_TagManager_RuntimeContract.md` for easy reference.
- Noted that the subsystem cleans itself up via `HandleActorEndPlay`, so there are no leaked handles/tags when actors drop out.
