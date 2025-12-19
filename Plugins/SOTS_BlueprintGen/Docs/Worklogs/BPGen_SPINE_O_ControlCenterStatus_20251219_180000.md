# BPGen_SPINE_O — Control Center status/monitoring refresh

## Goal
Add Control Center status snapshot and richer recent request monitoring for SPINE_O usability.

## Changes
- Control Center UI now separates status, messages, and recent requests; added Refresh Status button and newest-first recent list with request id/error/duration.
- Status view shows bind, safe mode, limits (bytes/RPS/RPM), uptime, start info, and enabled feature flags.
- Bridge server exposes `GetServerInfoForUI` to supply the snapshot; Control Center consumes it.
- Docs updated for the new status/monitoring surface.

## Verification
- Not built or run (editor-only changes).

## Notes / Risks
- UI assumes bridge server info is available in-process; no runtime validation yet.
- Recent request ordering and timings unverified in-editor.
