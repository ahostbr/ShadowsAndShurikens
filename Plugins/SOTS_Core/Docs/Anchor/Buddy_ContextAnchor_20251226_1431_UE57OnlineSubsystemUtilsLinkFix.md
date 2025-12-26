Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1431 | Plugin: SOTS_Core | Pass/Topic: UE57_OnlineSubsystemUtils_LinkFix | Owner: Buddy
Scope: Fix UE 5.7 link error for BuildPrimaryIdentity UniqueId ToString

DONE
- Added `OnlineSubsystemUtils` to `PrivateDependencyModuleNames` in `SOTS_Core.Build.cs`.

VERIFIED
- 

UNVERIFIED / ASSUMPTIONS
- Assumption: `FUniqueNetIdWrapper::ToString()` implementation is in `OnlineSubsystemUtils` for this UE build.
- UNVERIFIED: Needs an editor build to confirm link is fixed.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs
- Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_143100_UE57OnlineSubsystemUtilsLinkFix.md

NEXT (Ryan)
- Rebuild `ShadowsAndShurikensEditor`; confirm `UnrealEditor-SOTS_Core.dll` links successfully.

ROLLBACK
- Revert the dependency addition in `SOTS_Core.Build.cs`.
