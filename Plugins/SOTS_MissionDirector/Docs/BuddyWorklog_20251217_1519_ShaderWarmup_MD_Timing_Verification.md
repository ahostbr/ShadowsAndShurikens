# Buddy Worklog - ShaderWarmup MD Timing Verification (2025-12-17 15:19)

## Summary
- Rewired MissionDirector to travel on ShaderWarmup OnReadyToTravel (pre-teardown) so MoviePlayer persists through OpenLevel.
- Added dev-only proof logs for MD travel and UI teardown.

## Files touched
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupTypes.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_ShaderWarmupSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp

## Notes
- Tags checked in Config/DefaultGameplayTags.ini: required shader warmup tags present.
- No builds run.

## Cleanup
- Deleted Plugins/SOTS_MissionDirector/Binaries and Plugins/SOTS_MissionDirector/Intermediate
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate
