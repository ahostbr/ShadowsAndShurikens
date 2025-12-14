# Buddy Worklog — SOTS_GlobalStealthManager Profile Slice Wiring

1) Prompt / Task ID
- Wire GSM into profile snapshot pipeline with zero gameplay behavior change; ensure Build/Apply exist for FSOTS_GSMProfileData and hook through ProfileSubsystem.

2) Goal
- Let ProfileSubsystem serialize the GSM slice (FSOTS_GSMProfileData) via Build/Apply while keeping runtime stealth behavior unchanged.

3) What Changed
- Added no-op profile Build/Apply functions on `USOTS_GlobalStealthManagerSubsystem` that default/reset the GSM slice.
- Wired GSM into `USOTS_ProfileSubsystem` BuildSnapshotFromWorld/ApplySnapshotToWorld using the same subsystem access pattern as missions/music/FX.

4) Files Changed
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp
- Source/ShadowsAndShurikens/Private/SOTS_ProfileSubsystem.cpp
- Plugins/Docs/BuddyWorklog_20251213_141800_SOTS_GlobalStealthManager_ProfileSlice_Wiring.md

5) Notes / Decisions
- FSOTS_GSMProfileData currently carries alert/tier/persistent flags; per instructions, Build zeroes to defaults and Apply is a no-op (UE_UNUSED) to avoid altering live stealth state.
- SOTS_ProfileShared dependency already present in GSM Build.cs; added header include to expose the slice type.

6) Verification Notes
- Not built (per instructions). Pattern mirrors MMSS/MissionDirector subsystem calls in ProfileSubsystem; compile should succeed with existing module dependencies.

7) Cleanup Confirmation
- Pending: will delete Plugins/SOTS_GlobalStealthManager/Binaries and Intermediate after edits (no build triggered).

8) Follow-ups / TODOs
- If/when GSM state should persist (e.g., alert carryover), adjust Build/Apply to fill/restore fields and add versioning; keep Tag Spine rules in mind.
