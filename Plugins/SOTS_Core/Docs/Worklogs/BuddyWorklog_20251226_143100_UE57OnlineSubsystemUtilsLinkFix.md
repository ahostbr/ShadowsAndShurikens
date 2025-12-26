# Buddy Worklog — UE5.7 OnlineSubsystemUtils link fix

Goal
- Unblock UE 5.7.1 link for `SOTS_Core` by resolving the unresolved external on `FUniqueNetIdWrapper::ToString()` (observed from `USOTS_CoreLifecycleSubsystem::BuildPrimaryIdentity()`).

What changed
- Added `OnlineSubsystemUtils` to `SOTS_Core` module dependencies to ensure the implementation backing `FUniqueNetIdRepl::ToString()` is linked.

Files changed
- Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs

Notes / risks / unknowns
- UNVERIFIED: Depending on engine configuration, additional online modules could be required, but this is the minimal likely dependency based on the symbol.

Verification status
- UNVERIFIED: No UE build run here.

Next steps (Ryan)
- Rebuild `UnrealEditor-SOTS_Core.dll` and confirm the `LNK2019`/`LNK1120` is resolved.
