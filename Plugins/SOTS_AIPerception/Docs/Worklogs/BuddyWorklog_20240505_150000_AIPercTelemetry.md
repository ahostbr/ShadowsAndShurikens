# Buddy Worklog — AIPercTelemetry

## Goal
Add SPINE3 telemetry contract pieces to `SOTS_AIPerception` so Blueprint consumers can receive contextual detection/suspicion data with payloads and optional logging.

## Changes
- Added telemetry multicast delegates and payload plumbing on `USOTS_AIPerceptionComponent`.
- Captured last stimulus actor/location/reason during sight, shadow, and hearing inputs for telemetry snapshots.
- Implemented telemetry snapshot/broadcast helpers with optional debug logging gated by GuardConfig.
- Wired suspicion/detection change handling to emit telemetry delegates alongside legacy broadcasts.

## Files Touched
- `Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionComponent.h`
- `Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp`

## Verification
- No builds or automated tests run (per instructions).

## Cleanup
- Plugin had no `Binaries/` or `Intermediate/` to delete; nothing removed.

## Follow-ups
- Consider reconciling duplicated perception update paths to ensure `UpdateSuspicionModel` is the active flow for telemetry.
- Add Blueprint helper accessors if needed for external systems.
