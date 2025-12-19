[CONTEXT_ANCHOR]
ID: 20251219_185200 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenUIFix | Owner: Buddy
Scope: Fix UE5.7 compile errors in BPGen ControlCenter UI.

DONE
- Removed unsupported MinDesiredHeight from SMultiLineEditableTextBox widgets.
- Updated Json TryGetObjectField usage to pointer-return signature and FStringView.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Stub bridge backend remains; UI functionality still placeholder.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SSOTS_BPGenControlCenter.cpp

NEXT (Ryan)
- Re-run UBT for ShadowsAndShurikensEditor.
- If bridge functionality needed, implement real bridge server and adjust UI accordingly.

ROLLBACK
- Revert SSOTS_BPGenControlCenter.cpp to previous revision.
