Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251225_1953 | Plugin: SOTS_Core | Pass/Topic: UE57_UniqueId_ToString_Bypass | Owner: Buddy
Scope: UE 5.7.1 link unblock for UniqueId ToString unresolved external

DONE
- Replaced `FUniqueNetIdRepl::ToString()` usage in `USOTS_CoreLifecycleSubsystem::BuildPrimaryIdentity()` with a placeholder string when the ID is valid.
- Declared `OnlineSubsystem` + `OnlineSubsystemUtils` plugin dependencies in `SOTS_Core.uplugin`.
- Removed plugin `Binaries/` and `Intermediate/` folders.

VERIFIED
- UNVERIFIED (no build run in this pass).

UNVERIFIED / ASSUMPTIONS
- Assumed the UniqueId string is diagnostic-only; placeholder is acceptable for unblocking.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/SOTS_Core.uplugin

NEXT (Ryan)
- Rebuild editor target; confirm `FUniqueNetIdWrapper::ToString()` unresolved external is resolved.
- If more online-related link issues appear, reassess module/plugin deps or switch to an engine-header-safe UniqueId representation.

ROLLBACK
- Revert the two edited files.
