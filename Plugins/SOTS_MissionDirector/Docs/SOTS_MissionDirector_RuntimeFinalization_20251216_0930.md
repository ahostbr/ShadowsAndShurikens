# SOTS MissionDirector — Runtime Finalization (Prompt 6)

## Entry + validation hardening
- StartMission now refuses concurrent missions, validates mission data (ids present, routes/objectives non-null, unique ids, AllowedRoutes/RequiresCompleted point at known ids), and resets milestone/log bookkeeping per run.
- MissionId resolution prefers MissionIdentifier.Id, falling back to legacy MissionId before starting scoring/logging.

## Objective gating + timers
- Progress is ignored when route gating or prerequisite objectives are not satisfied for an objective.
- Objective terminal states clear any condition counters/timers for that objective to stop stale callbacks.
- Mission terminal states clear all condition tracking and pop any mission-specific stealth override.

## Persistence/UI guarantees
- Milestone history/logging flags reset on mission start so missing-sink logs remain one-shot per run.
- Route activation fallback still occurs automatically; UI intents/persistence writes remain on milestones only (start, route activated, objective terminal, mission terminal).

## Notes
- No new gameplay tags or Blueprint changes introduced.
- Next steps if needed: wire explicit route-switch entrypoint, add abort handling using the shared terminal cleanup, and expose validation results to tooling.
