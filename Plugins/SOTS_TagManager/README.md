# SOTS_TagManager

This plugin provides the centralized Tag Manager subsystem and blueprint surfaces for the live project.

SOTS TAG SPINE (V2)
- Purpose: This plugin is the single authoritative surface for shared loose gameplay tags for actors.
- API surface: `USOTS_GameplayTagManagerSubsystem` (C++/subsystem) and `USOTS_TagLibrary` (Blueprint wrappers).

Usage Guidelines:
- Do not mutate actor-owned tag containers directly in plugin code. Instead:
  1) Obtain the subsystem via `SOTS_GetTagSubSystem(WorldContextObject)` or call the `USOTS_TagLibrary` blueprint functions.
  2) Use `AddTagToActor`, `RemoveTagFromActor`, `AddTagToActorByName`, `RemoveTagFromActorByName` for writes.
  3) To query tags, use `ActorHasTag`, `ActorHasAnyTag`, and `ActorHasAllTags`.

CI & Developer Tools:
- There's a DevTools Python script `DevTools/python/check_tag_spine.py` that heuristically searches for `.AddTag` / `.RemoveTag` calls outside of this plugin. Use it to find probable ad-hoc writes.
- Use `DevTools/run_dependency_checks.ps1` to verify plugin dependencies and `DevTools/clean_plugin_artifacts.ps1` to clean compiled plugin artifacts.

Why this matters:
- Routing shared tag writes through an authoritative subsystem avoids latent inconsistency and ensures a consistent view of gameplay tags across plugins (GameInstance-scoped).

