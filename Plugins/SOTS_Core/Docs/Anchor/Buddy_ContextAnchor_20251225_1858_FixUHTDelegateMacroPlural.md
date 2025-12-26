Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1858 | Plugin: SOTS_Core | Pass/Topic: FixUHTDelegateMacroPlural | Owner: Buddy
Scope: Fix UHT-breaking delegate macro typos in lifecycle subsystem header.

DONE
- Fixed delegate macro typos in `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h`:
  - `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParam` -> `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams`
  - `DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParam` -> `DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams`
- Deleted `Plugins/SOTS_Core/Binaries/` and `Plugins/SOTS_Core/Intermediate/` (if present).

VERIFIED
- None.

UNVERIFIED / ASSUMPTIONS
- Assumption: This was purely a macro name typo; pluralized macros are the UE-supported forms and will satisfy UHT.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h
- Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251225_185800_FixUHTDelegateMacroPlural.md
- Plugins/SOTS_Core/Docs/Anchor/Buddy_ContextAnchor_20251225_1858_FixUHTDelegateMacroPlural.md

NEXT (Ryan)
- Re-run Editor build; success means UHT no longer errors on the delegate declaration line.

ROLLBACK
- Revert `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h` to restore previous macros (not recommended).
