Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1035 | Plugin: SOTS_Core | Pass/Topic: BRIDGE5_BridgeHealth | Owner: Buddy
Scope: Added bridge-focused diagnostics command `SOTS.Core.BridgeHealth`.

DONE
- Added `FSOTS_CoreDiagnostics::DumpBridgeHealth(UWorld*)` and registered new console command `SOTS.Core.BridgeHealth` (non-shipping/test).
- Updated SOTS_Core overview docs with Bridge Health section.

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- No build/editor run performed; output format/availability is inferred from code.
- Origin/type identification may be `UNKNOWN` depending on RTTI and implementation details.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Diagnostics/SOTS_CoreDiagnostics.h
- Plugins/SOTS_Core/Source/SOTS_Core/Private/Diagnostics/SOTS_CoreDiagnostics.cpp
- Plugins/SOTS_Core/Source/SOTS_Core/Private/SOTS_Core.cpp
- Plugins/SOTS_Core/Docs/SOTS_Core_Overview.md
- Plugins/SOTS_Core/Docs/Worklogs/BuddyWorklog_20251226_103500_SOTS_Core_BRIDGE5_BridgeHealth.md
- Plugins/SOTS_Core/Docs/Anchor/Buddy_ContextAnchor_20251226_1035_BRIDGE5_BridgeHealth.md

NEXT (Ryan)
- In-editor, run `SOTS.Core.BridgeHealth` in a PIE/world context and confirm it prints settings, listener list, save participant list, and snapshot summary.
- Toggle BRIDGE1–3 plugin settings and `bEnableLifecycleListenerDispatch` to confirm listener counts change as expected.

ROLLBACK
- Revert the files listed in FILES TOUCHED.
