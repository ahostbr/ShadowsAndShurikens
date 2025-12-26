# Buddy Worklog — 2025-12-26 08:44:33 — LifecycleHandleExport

## goal
Fix UE 5.7 Windows linker errors involving `FSOTS_CoreLifecycleListenerHandle` referenced by other modules (e.g., `SOTS_Input`, `SOTS_ProfileShared`).

## what changed
- Exported `FSOTS_CoreLifecycleListenerHandle` from the `SOTS_Core` module by adding the `SOTS_CORE_API` macro to the class declaration.

## files changed
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListenerHandle.h

## notes + risks/unknowns
- Root cause hypothesis: the handle’s methods are defined out-of-line in `SOTS_CoreLifecycleListenerHandle.cpp`; without `SOTS_CORE_API` the symbols are not exported from the `SOTS_Core` DLL on Windows, producing unresolved externals in dependent modules.
- `SOTS_Input.Build.cs` and `SOTS_ProfileShared.Build.cs` already list `SOTS_Core` as a dependency, so this looks like an export issue rather than a missing dependency.

## verification status
- UNVERIFIED: No build/run performed (per process). Change is compile/link intent only.

## follow-ups / next steps (Ryan)
- Rebuild the touched plugins/modules and confirm the previous unresolved external symbol errors for `FSOTS_CoreLifecycleListenerHandle` are gone.
- If any linker errors remain, capture the exact missing symbol names (decorated + undecorated) to verify whether they come from `SOTS_CoreLifecycleListenerHandle.cpp` or another unit.
