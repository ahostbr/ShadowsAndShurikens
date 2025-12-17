[CONTEXT_ANCHOR]
ID: 20251217_1551 | Plugin: SOTS_UI | Pass/Topic: WarmupDelegateInclude_v2 | Owner: Buddy
Scope: Fix missing CoreUObjectDelegates header by using UObjectGlobals where delegates live.

DONE
- Removed __has_include guard and now include UObject/UObjectGlobals.h directly (FCoreUObjectDelegates is declared there in UE 5.7p/5.7.1).

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Compile succeeds once header error is gone; runtime unchanged.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp

NEXT (Ryan)
- Rebuild SOTS_UI to ensure header error is resolved.

ROLLBACK
- Revert the cpp file to prior revision.
