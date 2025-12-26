Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_1335 | Plugin: SOTS_UI | Pass/Topic: BRIDGE10 SaveContractQuery | Owner: Buddy
Scope: Add OFF-by-default SOTS_UI ↔ SOTS_Core bridge seam to query Save Contract participants

DONE
- Added SOTS_UI settings: bEnableSOTSCoreSaveContractBridge (default false) and bEnableSOTSCoreSaveContractBridgeVerboseLogs (default false).
- Added USOTS_UIRouterSubsystem::QueryCanSaveViaCoreContract(FSOTS_SaveRequestContext, FText& OutBlockReason) that enumerates FSOTS_CoreSaveParticipantRegistry participants and returns false if any block.
- Added docs: SOTS_UI_SOTSCoreSaveContractBridge.md.

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Assumes participants are registered via ModularFeatures under SOTS.CoreSaveParticipant.
- No runtime/editor verification performed.

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UISettings.h
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Docs/SOTS_UI_SOTSCoreSaveContractBridge.md
- Plugins/SOTS_UI/Docs/Worklogs/BuddyWorklog_20251226_133500_BRIDGE10_SOTS_UI_SaveContractQuery.md
- Plugins/SOTS_UI/Docs/Anchor/Buddy_ContextAnchor_20251226_1335_BRIDGE10_SOTS_UI_SaveContractBridge.md

NEXT (Ryan)
- Build + run with BRIDGE10 ON; success = QueryCanSaveViaCoreContract returns expected results and does not alter existing save flows.

ROLLBACK
- Revert the touched files above, or set SOTS_UI bEnableSOTSCoreSaveContractBridge=false.
