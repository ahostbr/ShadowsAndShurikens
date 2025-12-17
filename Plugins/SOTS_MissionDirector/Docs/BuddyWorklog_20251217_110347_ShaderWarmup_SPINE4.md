# Buddy Worklog - SOTS_MissionDirector ShaderWarmup SPINE4 (2025-12-17 11:03:47)

**Goal**: Wire MissionDirector to start shader warmup, wait for completion, then travel.

**Changes**
- Added RequestMissionTravelWithWarmup entrypoint that builds a warmup request, binds OnFinished, and falls back to immediate travel if warmup can't start.
- Added HandleWarmupReadyToTravel callback to unbind and perform OpenLevel once warmup signals ready.
- Added CancelWarmupTravel for abort/back-out paths and cleanup on Deinitialize.

**Files touched**
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp

**Verification**
- Not run (code-only change; no build executed per SOTS law).

**Cleanup**
- Deleted Plugins/SOTS_MissionDirector/Binaries and Plugins/SOTS_MissionDirector/Intermediate after edits.
