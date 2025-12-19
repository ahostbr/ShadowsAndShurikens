[CONTEXT_ANCHOR]
ID: 20251219_2035 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenWidgetInclude | Owner: Buddy
Scope: Remove WidgetBlueprint include blocker from BPGen ensure helpers.

DONE
- Dropped Blueprint/WidgetBlueprint.h include and forward-declared UWidgetBlueprint to avoid missing header in UE5.7 layout.

VERIFIED
- None (not built/run).

UNVERIFIED / ASSUMPTIONS
- Expect C1083 on WidgetBlueprint.h to clear; further UWidgetBlueprint symbol issues may appear on link/compile.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenEnsure.cpp

NEXT (Ryan)
- Re-run UBT for editor; if new errors reference UWidgetBlueprint symbols, swap to appropriate UMG Editor header or adjust module dependencies.

ROLLBACK
- Revert SOTS_BPGenEnsure.cpp to restore prior include.
