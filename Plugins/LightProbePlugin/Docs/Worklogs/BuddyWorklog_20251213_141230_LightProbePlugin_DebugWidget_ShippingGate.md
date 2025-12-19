# Buddy Worklog — LightProbePlugin Shipping Gate for Debug Widget

1) Prompt / Task ID
- User request: gate LightProbe debug widget so it cannot ship; add runtime dev-only cvar for opt-in; clean plugin artifacts; document work.

2) Goal
- Prevent LightProbe debug viewport widget from appearing in Shipping/Test builds and keep it off by default in development unless explicitly enabled.

3) What Changed
- Added dev-only console cvar `lightprobe.DebugWidget` (default 0) to gate runtime debug widget usage.
- Wrapped debug widget spawn in `#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)` and required the cvar + `bShowDebugWidget` before creating the widget; prevented duplicate spawns.

4) Files Changed
- Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp
- Plugins/Docs/BuddyWorklog_20251213_141230_LightProbePlugin_DebugWidget_ShippingGate.md

5) Notes / Decisions
- The cvar exists only in non-Shipping/Test builds; description kept short to avoid log spam.
- No auto-hide sink added; widget only spawns when both the cvar and component flag are true during initialization. Shutdown still removes the widget cleanly.

6) Verification Notes
- Not built (per instructions). Reasoning: compile guard excludes Shipping/Test; runtime gating requires `lightprobe.DebugWidget` to be non-zero and `bShowDebugWidget` set.

7) Cleanup Confirmation
- Removed plugin build artifacts after edits (Plugins/LightProbePlugin/Binaries, Plugins/LightProbePlugin/Intermediate).

8) Follow-ups
- If runtime toggling of the cvar is desired to auto-hide an existing widget, add a console sink that clears/removes the widget when the cvar goes to 0.
