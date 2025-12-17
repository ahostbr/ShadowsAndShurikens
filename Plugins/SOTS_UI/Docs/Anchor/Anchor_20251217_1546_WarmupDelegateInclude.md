[CONTEXT_ANCHOR]
ID: 20251217_1546 | Plugin: SOTS_UI | Pass/Topic: WarmupDelegateInclude | Owner: Buddy
Scope: ShaderWarmup subsystem include path compatibility for FCoreUObjectDelegates on UE 5.7p.

DONE
- Added __has_include guard in SOTS_ShaderWarmupSubsystem.cpp to include UObject/CoreUObjectDelegates.h or flat CoreUObjectDelegates.h; errors out if neither exists.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Header exists under one of the guarded names in UE 5.7p; behavior unchanged.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp

NEXT (Ryan)
- Rebuild on UE 5.7p to confirm SOTS_UI compiles.
- If failure persists, note exact header path/name in UE 5.7p so we can add it to the include guard.

ROLLBACK
- Revert the cpp file to the prior revision.
