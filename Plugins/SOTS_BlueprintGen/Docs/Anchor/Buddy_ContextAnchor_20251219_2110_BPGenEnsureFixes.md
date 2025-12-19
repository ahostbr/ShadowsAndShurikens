[CONTEXT_ANCHOR]
ID: 20251219_2110 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenEnsureFixes | Owner: Buddy
Scope: Fix ensure.cpp compile errors (UE5.7 API drift).

DONE
- Added SavePackage/property/widget/K2 node headers; reintroduced concrete UWidgetBlueprint usage.
- Swapped ImportText to ImportText_Direct.
- Spawn widget trees with explicit class/flags to avoid incomplete type issues.
- Updated ChangeMemberVariableType usage (void) and metadata keys to string literals.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- Expect ensure.cpp errors to clear; further UMG header path issues possible if engine layout differs.

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenEnsure.cpp

NEXT (Ryan)
- Re-run UBT for editor; share new log if any remaining errors (UMG types/metadata keys).

ROLLBACK
- Revert SOTS_BPGenEnsure.cpp to prior revision.
