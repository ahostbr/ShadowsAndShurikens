# Buddy Worklog — SOTS_Interaction — SPINE 1 (Plugin Skeleton)

## What changed
- Created new Runtime plugin: `Plugins/SOTS_Interaction`
- Added module wiring: `.uplugin`, `Build.cs`, module startup/shutdown implementation
- Added log category `LogSOTSInteraction`

## Files added
- Plugins/SOTS_Interaction/SOTS_Interaction.uplugin
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/SOTS_Interaction.Build.cs
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionModule.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionModule.cpp
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionLog.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionLog.cpp

## Notes / Assumptions
- Plugin is Runtime-only and currently depends on `SOTS_TagManager`.
- No UI, widgets, or SOTS_UI integration in this pass (skeleton only).

## TODO (next pass)
- Add core data contracts: InteractionContext/Option, InteractableComponent, InteractableInterface.
- Add InteractionSubsystem (GameInstanceSubsystem) in subsequent pass.
