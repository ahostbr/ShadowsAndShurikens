# SOTS Interaction — SPINE4 Intent Diagnostics

## Summary
- Added dev-only diagnostics so UI intents cannot silently drop: throttled warnings for unbound listeners, optional payload logging, and legacy delegate warnings (all default OFF).
- Kept UI intent contract unchanged; broadcasts and payload types are untouched.

## Intents and broadcasts
- Primary delegate: `OnUIIntentPayload` (payload `FSOTS_InteractionUIIntentPayload`), broadcast in `BroadcastUIIntent`.
- Legacy delegate: `OnUIIntent` (deprecated), broadcast in `BroadcastUIIntent` for backward compatibility.

## New config flags (default false)
- `bWarnOnUnboundInteractionIntents` — warn when no listeners are bound to UI intents (throttled).
- `bWarnOnLegacyIntentUsage` — warn when the legacy intent delegate is used (throttled).
- `bDebugLogIntentPayloads` — verbose log payload details for intents (dev-only).
- `WarnIntentCooldownSeconds` — throttle interval for repeated warnings.

## Throttling strategy
- Warnings are gated by cooldown timers (`LastWarnUnboundIntentTime`, `LastWarnLegacyIntentTime`) and only active in non-shipping/test builds.

## Evidence
- Config flags and cooldown fields: `SOTS_InteractionSubsystem` ([Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h](Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h)).
- Unbound and payload logging + legacy warnings at broadcast site: `BroadcastUIIntent` in `SOTS_InteractionSubsystem.cpp` ([Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp#L889-L934](Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp#L889-L934)).

## Verification
- Not run (per instructions).

## Cleanup
- Removed `Plugins/SOTS_Interaction/Binaries/` and `Plugins/SOTS_Interaction/Intermediate/` after edits.
