# Buddy Worklog - MMSS Resume and Persistence Sweep

Goal: finalize the MMSS resume/persistence behavior so the system respects TrackId without a mission, treats stored playback time as “close enough,” and keeps the single-layer state deterministic.

Changes:
- Default-music requests now honor the requested `TrackId` and only fall back to the first default entry when the requested tag is absent, keeping the role-intent fields stable for profiling/snapshots.
- Resume flagging is wired into `RequestRoleTrack`/`ApplyProfileData` and `HandleLoadedSound` so stored `LastPlaybackTimeSeconds` is replayed as a tolerant “close enough” resume point while still recording state through the existing snapshot fields.
- Updated `SOTS_MMSS_ResumePolicy.md` to describe the TrackId-first default lookup and the resume flag lifecycle so future work understands the single-layer constraint.

Files touched:
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSSubsystem.cpp
- Plugins/SOTS_MMSS/Source/SOTS_MMSS/Public/SOTS_MMSSSubsystem.h
- Plugins/SOTS_MMSS/Docs/SOTS_MMSS_ResumePolicy.md

Notes:
- MMSS continues to register as a profile provider; it never writes snapshots itself, so persistence remains owned by `SOTS_ProfileShared`.

Cleanup:
- Deleted Plugins/SOTS_MMSS/Binaries and Plugins/SOTS_MMSS/Intermediate.
