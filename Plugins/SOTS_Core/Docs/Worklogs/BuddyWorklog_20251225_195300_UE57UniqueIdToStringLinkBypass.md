# Buddy Worklog — 2025-12-25 19:53 — UE5.7 UniqueId ToString link bypass

## Goal
Unblock UE 5.7.1 link for `SOTS_Core` by removing the dependency on the unresolved symbol `FUniqueNetIdWrapper::ToString()` referenced from `USOTS_CoreLifecycleSubsystem::BuildPrimaryIdentity()`.

## What changed
- Replaced `UniqueId.ToString()` with a constant placeholder string (`"Valid"`) when `UniqueId.IsValid()` is true.
  - This keeps the identity field populated without calling into the missing symbol.
- Updated `SOTS_Core.uplugin` to declare plugin dependencies on `OnlineSubsystem` and `OnlineSubsystemUtils` to match the module dependency warnings.

## Files changed
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Subsystems/SOTS_CoreLifecycleSubsystem.cpp
- Plugins/SOTS_Core/SOTS_Core.uplugin

## Notes / Risks / Unknowns
- **Behavior change:** `PlayerStateUniqueIdString` is no longer the actual net id string; it is now a placeholder.
- If downstream logic relies on the exact unique id string, we may need a better UE5.7-safe representation (e.g., replication bytes) once the build is unblocked.

## Verification status
UNVERIFIED
- No build/link run in this pass.
- Plugin `Binaries/` and `Intermediate/` folders were removed to force a clean rebuild.

## Next steps (Ryan)
- Rebuild `ShadowsAndShurikensEditor` and confirm the link error on `FUniqueNetIdWrapper::ToString()` is gone.
- If you need the real ID string, we can iterate on a replacement that doesn’t require that symbol (engine-header dependent).
