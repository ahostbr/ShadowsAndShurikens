# SOTS_AIPerception SPINE5 — Lifecycle Policy (Stateless)

## Goal
Lock AI perception lifecycle to avoid sticky detection across transitions and expose deterministic reset hooks (BeginPlay/EndPlay + mission/profile helpers).

## Policy Decision
- **Chosen:** Stateless (no snapshot/save code exists in plugin; only runtime caches). No persistence expectations were found in AIBT BEP exports (searches for Save/Profile/Persist yielded only animation blend profiles, not perception state).

## Reset API & Reasons
- `ESOTS_AIPerceptionResetReason` now covers: Unknown, BeginPlay, EndPlay, TargetChanged, MissionStart, ProfileLoaded, Manual.
- `ResetPerceptionState(Reason)` clears runtime caches (suspicion/detected, timers/gates, last stimulus, watched targets, primary target, LOS/perception tags, trace budgets, telemetry schedule, GSM throttle state, cached GSM ref, FX curve cache) and reapplies Unaware/LOS tags when needed.
- Convenience BPs: `ResetForProfileLoaded()`, `ResetForMissionStart()` forward to `ResetPerceptionState` with explicit reasons.
- Last reset reason tracked internally for diagnostics.

## Lifecycle Hooks
- BeginPlay: calls `ResetPerceptionState(BeginPlay)` before starting timers.
- EndPlay: unregisters from subsystem, clears timers, and resets state (EndPlay reason).
- OnUnregister: defensive clear + reset (EndPlay reason) to prevent sticky state during teardown.
- Manual calls (profile/mission) available to external systems; no new inter-plugin deps were introduced.

## AIBT BEP Notes
- AIBT assets consume `TargetLocation`/behavior component data; no persistence or save/profile hooks surfaced in BEP exports. This supports stateless perception between missions/levels.

## Verification
- No builds/tests run (per instruction).
- Plugin artifacts cleaned: Plugins/SOTS_AIPerception/Binaries and Intermediate removed.

## Evidence Pointers
- Reset reasons enum: Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L20-L28.
- Blueprint reset helpers (mission/profile): Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L90-L107.
- Lifecycle hooks + OnUnregister reset: Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L20-L55.
- ResetInternalState clears suspicion/detected/stimulus/GSM caches and tracks last reason: Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L170-L215.
- Last reset reason field for diagnostics: Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h#L160-L165.
