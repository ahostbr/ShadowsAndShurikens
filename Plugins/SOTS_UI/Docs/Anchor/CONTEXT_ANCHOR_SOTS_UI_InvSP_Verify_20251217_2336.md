[CONTEXT_ANCHOR]
ID: 20251217_2336 | Plugin: SOTS_UI | Pass/Topic: InvSP_Adapter_ContainerRoute | Owner: Buddy
Scope: Keep InvSP flows adapter-driven and confirm shader warmup fallback stays level-scoped.

DONE
- Added container entrypoints to InvSP adapter (OpenContainer/CloseContainer) to keep container menu flows on the adapter surface.
- Router now calls InvSP adapter container entrypoints before container widget push/pop to preserve InvSP-first sequencing.

VERIFIED
- None (no build/run).

UNVERIFIED / ASSUMPTIONS
- InvSP adapter implementation will wire the new container entrypoints; runtime behavior not observed.
- Shader warmup fallback remains scoped to target-level dependencies (no code changes made).

FILES TOUCHED
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_InvSPAdapter.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_InvSPAdapter.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp

NEXT (Ryan)
- Implement/wire adapter container entrypoints (BP/C++) and rebuild SOTS_UI on UE5.7p to confirm signatures compile.
- Verify InvSP inventory/container open/close via adapter still works and router stack bookkeeping remains correct; ensure Back/Escape does not auto-close InvSP menus.
- Spot-check shader warmup fallback in editor/runtime to confirm the asset queue remains limited to the target level dependencies.

ROLLBACK
- Revert the three files listed above to remove the adapter container entrypoints and router calls.
