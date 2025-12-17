[CONTEXT_ANCHOR]
ID: 20250208_1542 | Plugin: SOTS_MissionDirector | Pass/Topic: WarmupCancelSignature | Owner: Buddy
Scope: Align warmup cancellation delegate signature with shader warmup sparse dynamic delegate.

DONE
- Updated HandleWarmupCancelled to take FText by value in header and cpp.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Signature change resolves delegate binding compile error; cancel reason still propagated correctly.

FILES TOUCHED
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Public/SOTS_MissionDirectorSubsystem.h
- Plugins/SOTS_MissionDirector/Source/SOTS_MissionDirector/Private/SOTS_MissionDirectorSubsystem.cpp

NEXT (Ryan)
- Rebuild to ensure delegate binding now matches OnCancelled signature.
- Trigger warmup cancel path to confirm reason text flows to mission director events.

ROLLBACK
- Revert the two touched files.