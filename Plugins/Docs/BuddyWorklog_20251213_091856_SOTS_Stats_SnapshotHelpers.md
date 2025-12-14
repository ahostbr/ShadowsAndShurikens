# Prompt ID
- Not provided.

# Goal
- Add canonical snapshot helpers so stats persist through `FSOTS_CharacterStateData.StatValues` (profile pipeline only).

# What changed
- Added `WriteToCharacterState` and `ReadFromCharacterState` BlueprintCallable helpers on `USOTS_StatsComponent` to copy stat values to/from `FSOTS_CharacterStateData.StatValues` without mutation or extra storage.
- Left existing logic untouched; no new save files or caches.

# Files changed
- Plugins/SOTS_Stats/Source/SOTS_Stats/Public/SOTS_StatsComponent.h
- Plugins/SOTS_Stats/Source/SOTS_Stats/Private/SOTS_StatsComponent.cpp

# Notes / Decisions
- Helpers perform direct map copies; no clamps, no broadcasts, no defaults injected when input is empty.
- Module already depends on `SOTS_ProfileShared` and includes `SOTS_ProfileTypes.h`, so no build rule changes were needed.

# Verification
- Not run (no builds or automated tests).

# Cleanup confirmation
- Deleted Plugins/SOTS_Stats/Binaries and Plugins/SOTS_Stats/Intermediate; no build triggered.

# Follow-ups
- Consider updating profile usage to call the new helpers explicitly, but existing Build/Apply functions remain available.
