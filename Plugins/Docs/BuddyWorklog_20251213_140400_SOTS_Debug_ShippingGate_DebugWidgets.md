# Buddy Worklog — SOTS_Debug Shipping Gate for Debug Widgets

1) Prompt / Task ID
- User request: gate SOTS_Debug widget spawning behind Shipping/Test compile guard and runtime cvar (sots.DebugWidgets), keep DEBUG UI BYPASS exception intact.

2) Goal
- Ensure debug widget spawning (KEM anchor overlay) is compiled out in Shipping/Test and is off by default in dev builds, with a cvar-controlled opt-in.

3) What Changed
- Added dev-only console cvar `sots.DebugWidgets` (default 0) and sink to hide widgets when toggled off.
- Wrapped KEM anchor overlay spawn/show/hide flows in `#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)` and runtime cvar gate.
- Added safe hides when cvar is disabled; no logging spam, null-safe controller checks preserved.

4) Files Changed
- Plugins/SOTS_Debug/Source/SOTS_Debug/Private/SOTS_SuiteDebugSubsystem.cpp
- Plugins/Docs/BuddyWorklog_20251213_140400_SOTS_Debug_ShippingGate_DebugWidgets.md

5) Notes / Decisions
- Cvar lives only in non-Shipping/Test builds to avoid shipping surface; uses `TAutoConsoleVariable` + sink for 1→0 auto-hide.
- Kept existing toggle API; now ANDed with cvar and compile guard. No new dependencies added.

6) Verification Notes
- Not built (per instructions). Reasoning: compile guards ensure Shipping/Test exclusion; dev runtime path checks `sots.DebugWidgets` before spawn and sink hides on disable.

7) Cleanup Confirmation
- Removed plugin build artifacts after edits (Plugins/SOTS_Debug/Binaries, Plugins/SOTS_Debug/Intermediate).

8) Follow-ups
- None pending; user mentioned LightProbePlugin gating to be addressed separately.
