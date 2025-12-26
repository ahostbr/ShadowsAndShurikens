# Buddy Worklog 2025-12-25 - Fix UHT Delegate Macro Pluralization

## Goal
Unblock UHT/header parse failure:
- `Unable to parse delegate declaration; expected 'DECLARE_DYNAMIC_MULTICAST_DELEGATE' but found 'DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParam'.`

## Evidence
From `Saved/Logs/ShadowsAndShurikens.log`:
- `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h(78): Error: Unable to parse delegate declaration; expected 'DECLARE_DYNAMIC_MULTICAST_DELEGATE' but found 'DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParam'.`

## What changed
Fixed typo(s) in delegate macro names in `SOTS_CoreLifecycleSubsystem.h`:
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParam` → `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams`
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParam` → `DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams`

These macros are pluralized in UE headers; the singular forms are not recognized by UHT.

Also deleted `SOTS_Core` build artifacts once (if present) to prevent stale intermediates:
- `Plugins/SOTS_Core/Binaries/`
- `Plugins/SOTS_Core/Intermediate/`

## Files touched
- Modified:
  - `Plugins/SOTS_Core/Source/SOTS_Core/Public/Subsystems/SOTS_CoreLifecycleSubsystem.h`
- Deleted (if present):
  - `Plugins/SOTS_Core/Binaries/`
  - `Plugins/SOTS_Core/Intermediate/`

## Verification status
- UNVERIFIED: No build/editor run performed on my side.

## Next steps (Ryan)
1. Re-run the Editor target build.
2. Confirm UHT progresses past the delegate parse error.
3. If a new UHT error appears, paste the exact `...h(line): Error:` line(s).
