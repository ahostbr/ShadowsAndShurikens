# Buddy Worklog — 20251226_153422 — Log-driven build fixes (BRIDGE series follow-up)

## Goal
Read the latest editor/build log and address any **build-stopping** issues introduced or revealed during the BRIDGE1–20 integration work, while keeping changes minimal and add-only where applicable.

## Evidence (log)
- Log: `Saved/Logs/ShadowsAndShurikens.log`
- Build failures observed:
  - `SOTS_CoreDiagnostics.cpp`: `PLATFORM_COMPILER_HAS_RTTI` macro used in `#if` caused `C4668` (treated as error).
  - `SOTS_MMSSCoreLifecycleHook.h`: includes `Interfaces/SOTS_CoreLifecycleListener.h` which does not exist.
  - `SOTS_UIModule.cpp`: compile errors from passing `AHUD*` to `USOTS_UIRouterSubsystem::Get(const UObject*)` and `GetNameSafe(HUD)` due to incomplete `AHUD` type.
- UBT dependency warnings observed:
  - `SOTS_ProfileShared` module depends on `SOTS_Core`, but plugin did not list `SOTS_Core`.
  - `SOTS_Input` module depends on `SOTS_Core`, but plugin did not list `SOTS_Core`.
  - `SOTS_MissionDirector` module depends on `SOTS_Stats`, but plugin did not list `SOTS_Stats`.
  - `SOTS_Steam` module depends on `SOTS_MissionDirector`, but plugin did not list `SOTS_MissionDirector`.

## What changed
### Build-stopping fixes
- SOTS_Core diagnostics RTTI guard:
  - Replaced `#if PLATFORM_COMPILER_HAS_RTTI` with `#if defined(PLATFORM_COMPILER_HAS_RTTI) && PLATFORM_COMPILER_HAS_RTTI` to avoid `C4668` when the macro is not defined.
- SOTS_MMSS include path:
  - Updated include to the correct public header path: `Lifecycle/SOTS_CoreLifecycleListener.h`.
- SOTS_UI module include:
  - Included `GameFramework/HUD.h` so `AHUD` is a complete type (enables implicit conversion to `UObject*` and makes `GetNameSafe(HUD)` resolve correctly).

### Dependency declarations (add-only)
- Added missing plugin dependency declarations in `.uplugin` files:
  - `SOTS_Input` now lists `SOTS_Core`.
  - `SOTS_ProfileShared` now lists `SOTS_Core`.
  - `SOTS_MissionDirector` now lists `SOTS_Stats`.
  - `SOTS_Steam` now lists `SOTS_MissionDirector`.

## Files changed/created
- Modified:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp`
  - `Plugins/SOTS_MMSS/Source/SOTS_MMSS/Private/SOTS_MMSSCoreLifecycleHook.h`
  - `Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIModule.cpp`
  - `Plugins/SOTS_Input/SOTS_Input.uplugin`
  - `Plugins/SOTS_ProfileShared/SOTS_ProfileShared.uplugin`
  - `Plugins/SOTS_MissionDirector/SOTS_MissionDirector.uplugin`
  - `Plugins/SOTS_Steam/SOTS_Steam.uplugin`
- Created:
  - `Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_153422_LogDrivenBuildFixes_BRIDGE_Series.md`

## Constraints check
- Add-only posture maintained for dependency fixes (added plugin dependency entries, no destructive renames/removals).
- No Blueprint edits.
- No Unreal build/run performed by Buddy in this prompt.

## Cleanup status (Binaries/Intermediate)
- Not re-attempted for `SOTS_Input/Intermediate` in this prompt (a prior one-time deletion attempt failed due to a locked UHT-generated file; per SOTS law, do not retry).
- Other touched plugins were previously cleaned during bridge passes; current on-disk status was not re-verified as part of this prompt.

## Verification status
- UNVERIFIED: compilation/editor startup after these fixes (Ryan-run).

## Next steps (Ryan)
- Re-run build (or start editor) and confirm the following are resolved:
  - `SOTS_CoreDiagnostics.cpp` no longer errors on RTTI macro guards.
  - `SOTS_MMSS` compiles with corrected listener include.
  - `SOTS_UI` compiles (no `AHUD*` conversion / `GetNameSafe` errors).
  - UBT plugin dependency warnings are reduced/removed.
- If desired: decide whether to normalize `.uplugin` `EngineVersion` fields (many show `"5.7"` parse warnings in the log; not changed here).
