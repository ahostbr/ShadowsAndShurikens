# Buddy Worklog — UE5.7 OnlineSubsystem module dep

Goal
- Continue unblocking UE 5.7.1 link for `SOTS_Core` after `OnlineSubsystemUtils` alone did not appear to resolve the unresolved external for `FUniqueNetIdWrapper::ToString()`.

What changed
- Added `OnlineSubsystem` to `SOTS_Core` `PrivateDependencyModuleNames` alongside `OnlineSubsystemUtils`.

Files changed
- Plugins/SOTS_Core/Source/SOTS_Core/SOTS_Core.Build.cs

Notes / risks / unknowns
- UNVERIFIED: The previous link error may have been from stale artifacts; this change aims to ensure the owning module for the symbol is linked.
- If the symbol still fails to resolve, the next step is likely either:
  - add another online module dependency based on the next linker output, or
  - avoid calling `FUniqueNetIdRepl::ToString()` in `USOTS_CoreLifecycleSubsystem::BuildPrimaryIdentity()`.

Verification status
- UNVERIFIED: No engine build run here.

Next steps (Ryan)
- Rebuild `UnrealEditor-SOTS_Core.dll` and confirm the `LNK2019`/`LNK1120` is resolved.
