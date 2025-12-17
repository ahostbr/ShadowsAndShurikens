# SOTS_UI Shader Warmup Cancel

## Who can cancel
- MissionDirector (travel back-out or flow change).
- UI flow (explicit cancel button or system action).

## What cancel guarantees
- Stops async loading and compile polling.
- Restores global time dilation.
- Stops MoviePlayer if it was active.
- Pops the warmup loading screen from the router.
- Broadcasts `OnCancelled` with the provided reason.

## Behavior note
- Cancel does not trigger mission travel; travel only happens after `OnFinished`.
