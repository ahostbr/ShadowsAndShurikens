# SOTS UI Router Interaction Verb Policy

## Scope
- The router owns UI flows such as pickup notifications, inventory widgets, modal dialogs, and explicit interaction prompts.
- Interaction verbs are routed by gameplay subsystems (SOTS_Interaction -> SOTS_INV / SOTS_BodyDrag / SOTS_KillExecutionManager); the router does not dispatch action requests.
- The router may log unhandled verbs for debugging but stays intent-only.

## Logging policy
- Log output from the router stays silent unless explicit debugging is requested.
- `sots.ui.LogUnhandledInteractionVerbs` (CVar, default `0`) enables at-most-once-per-verb logging as soon as the router receives a verb it cannot handle.
- When the CVar is `0` (default) “stub” messages are suppressed so playtests avoid log spam.
- When the CVar is `1`, each verb is logged once per session under `LogSOTS_UIRouter`, and the log clarifies that the verb is handled outside of the router for future wiring.

## Debugging steps
1. Enable `sots.ui.LogUnhandledInteractionVerbs` (set to `1` via console or ini).
2. Play until the router sees an unhandled verb; the log entry will mention the verb and the owning systems.
3. Reset the CVar or restart the session to allow the verb to log again in the next run.

## Reminder
- Preserve existing return/flow expectations for callers; the router treats all action verbs as no-ops and the caller flow does not change.
