Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_0844 | Plugin: SOTS_Core | Pass/Topic: LifecycleHandleExport | Owner: Buddy
Scope: Export fix for FSOTS_CoreLifecycleListenerHandle linker errors

DONE
- Added `SOTS_CORE_API` to `FSOTS_CoreLifecycleListenerHandle` class declaration.

VERIFIED
- (none)

UNVERIFIED / ASSUMPTIONS
- Assumed unresolved externals were caused by missing symbol exports from `SOTS_Core` on Windows.

FILES TOUCHED
- Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListenerHandle.h

NEXT (Ryan)
- Build UE 5.7 project/plugins and confirm linker errors for `FSOTS_CoreLifecycleListenerHandle` are resolved.
- If not resolved: paste the exact linker error lines so we can map the missing symbols to the correct module and apply the minimal follow-up.

ROLLBACK
- Revert the change in Plugins/SOTS_Core/Source/SOTS_Core/Public/Lifecycle/SOTS_CoreLifecycleListenerHandle.h
