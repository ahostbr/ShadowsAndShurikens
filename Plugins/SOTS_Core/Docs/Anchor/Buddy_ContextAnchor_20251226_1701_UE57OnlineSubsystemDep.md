Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1701 | Plugin: SOTS_Core | Pass/Topic: UE57 OnlineSubsystem Dep | Owner: Buddy
Scope: UE 5.7.1 link unblock attempt for `FUniqueNetIdWrapper::ToString()` unresolved external

DONE
- Added `OnlineSubsystem` to `PrivateDependencyModuleNames` in `SOTS_Core.Build.cs` (kept `OnlineSubsystemUtils`).

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Assumed the unresolved `FUniqueNetIdWrapper::ToString()` symbol is owned by a module now linked by `OnlineSubsystem`.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs

NEXT (Ryan)
- Rebuild `UnrealEditor-SOTS_Core.dll`; confirm the `LNK2019`/`LNK1120` is resolved.
- If still failing, consider adjusting `USOTS_CoreLifecycleSubsystem::BuildPrimaryIdentity()` to avoid `FUniqueNetIdRepl::ToString()` and/or add additional online module deps based on the new linker output.

ROLLBACK
- Revert `SOTS_Core.Build.cs`.
