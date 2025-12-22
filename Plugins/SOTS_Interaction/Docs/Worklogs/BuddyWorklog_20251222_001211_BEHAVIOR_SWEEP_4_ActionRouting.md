# Buddy Worklog - BEHAVIOR SWEEP 4 ActionRouting (20251222_001211)

Goal
- Ensure canonical interaction verbs route through the action request seam and are consumed by their owning subsystems.

What changed
- Action request payload now carries option MetaTags and optional ExecutionTag for Execute verbs; pickup metadata pulled from InteractableComponent when present.
- Interaction subsystem routes Pickup -> SOTS_INV, Execute -> KEM blessed entrypoint, DragStart/DragStop -> BodyDrag player component.
- UI router no longer dispatches action requests (intent-only), but can still log unhandled verbs for debugging.
- Updated seam/policy docs and added SOTS_Interaction plugin dependencies for INV/KEM/BodyDrag.

Files changed
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionTypes.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/SOTS_Interaction.Build.cs
- Plugins/SOTS_Interaction/SOTS_Interaction.uplugin
- Plugins/SOTS_Interaction/Docs/SOTS_Interaction_SPINE4_CrossPluginSeam.md
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Docs/SOTS_UIRouter_InteractionVerbPolicy.md
- Plugins/SOTS_Interaction/Docs/Worklogs/BuddyWorklog_20251222_001211_BEHAVIOR_SWEEP_4_ActionRouting.md

Notes + risks/unknowns
- Execute routing requires a valid ExecutionTag (sourced from InteractionTypeTag); if absent, the action is skipped with a Verbose log.
- Pickup routing skips when ItemTag is missing; action handlers tolerate missing values without crashing.

Verification
- Not run (no builds or runtime tests per guardrails).

Cleanup
- Delete Binaries/ and Intermediate/ for touched plugins (SOTS_Interaction, SOTS_UI).
