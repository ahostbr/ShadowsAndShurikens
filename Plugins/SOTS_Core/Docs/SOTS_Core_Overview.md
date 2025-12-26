# SOTS_Core Overview

## Purpose
SOTS_Core provides a minimal C++ spine (GameMode/GameState/PlayerState/PlayerController/HUD/PlayerCharacter)
plus a lifecycle event bus (GameInstanceSubsystem) so other SOTS plugins can bind to real engine lifecycle
signals without guessing or relying on UI adapters. This is additive only and does not change behavior until
Blueprints are intentionally reparented.

## Non-goals
- No gameplay or UI behavior changes in this pass.
- No Blueprint rewrites or forced adoption.
- No new dependencies on other SOTS plugins.

## Golden path adoption (later)
Reparent the existing CGF Blueprints when you are ready:
- BP_CGF_GameMode -> ASOTS_GameModeBase
- BP_CGF_GameState -> ASOTS_GameStateBase
- BP_CGF_PlayerState -> ASOTS_PlayerStateBase (optional)
- BP_CGF_PlayerController -> ASOTS_PlayerControllerBase
- BP_CGF_HUD -> ASOTS_HUDBase
- BP_CGF_Player -> ASOTS_PlayerCharacterBase

## Integration guidance for other plugins
- Prefer subscribing to USOTS_CoreLifecycleSubsystem events instead of casting to project BPs.
- Keep consumption optional; do not add hard dependencies on SOTS_Core in this pass.

## SPINE2: Golden hooks (optional, default OFF)
SOTS_Core can optionally bridge engine/world delegates before any Blueprint reparenting happens.
All toggles default to OFF so behavior stays unchanged until explicitly enabled.

### Settings (Config=Game)
- bEnableWorldDelegateBridge: when true, forwards world lifecycle delegates into the subsystem.
- bEnableVerboseCoreLogs: when true, allows Verbose lifecycle logs (still Verbose only).
- bSuppressDuplicateNotifications: when true (default), avoids duplicate broadcasts.

### World Delegate Bridge (pre-reparent phase)
When enabled, SOTS_Core listens to safe engine/world delegates and forwards them into the lifecycle bus.
This provides a minimal spine for other plugins without forcing any Blueprint changes.

### Snapshot usage
Use USOTS_CoreLifecycleSubsystem::GetCurrentSnapshot (or the Blueprint library) to read the current
World/PC/Pawn/HUD pointers without performing any gameplay actions.

## SPINE3: Lifecycle Listener Dispatch (optional, default OFF)
SOTS_Core can dispatch lifecycle events to lightweight C++ listeners registered via the modular
feature system. This enables other plugins (SOTS_Input, SOTS_ProfileShared, etc.) to receive
deterministic UE-native callbacks without adapters or hard dependencies.

### How it works
- Implement ISOTS_CoreLifecycleListener in another plugin.
- Register/unregister via IModularFeatures (or use the optional RAII handle in SOTS_Core).
- Enable dispatch in settings (bEnableLifecycleListenerDispatch).
- Optional: enable bEnableLifecycleDispatchLogs for Verbose per-listener dispatch logs.

### Why this is safe
- SOTS_Core does not depend on the listening plugins.
- Dispatch is disabled by default, so behavior stays inert until explicitly enabled.

## SPINE4: Map/Travel Bridge (optional, default OFF)
SOTS_Core can optionally forward map/travel delegates so plugins can observe real engine travel
events before any Blueprint reparenting. This is disabled by default.

### Settings
- bEnableMapTravelBridge: enables map/travel delegate binding.
- bEnableMapTravelLogs: Verbose-only logging for map/travel events.

### Events surfaced
- PreLoadMap (map name)
- PostLoadMapWithWorld (world pointer)
- WorldCleanup (world + flags)
- PostWorldInitialization (world pointer)

## SPINE5: Primary Player Identity (optional cache, default ON)
SOTS_Core provides a lightweight primary-player identity snapshot without OnlineSubsystem
dependencies, suitable for save/profile folder decisions.

### Identity fields
- LocalPlayerIndex (default 0)
- ControllerId (optional, INDEX_NONE when unavailable)
- PlayerName (from PlayerState when available)
- PlayerStateUniqueIdString (from PlayerState UniqueId when valid)

### Settings
- bEnablePrimaryIdentityCache: caches identity into the lifecycle snapshot (default ON).

## SPINE7: Diagnostics (command-invoked only)
SOTS_Core provides console commands for quick health checks without introducing any runtime spam.

### Console commands
- SOTS.Core.DumpSettings: logs SOTS_Core settings toggles.
- SOTS.Core.DumpLifecycle: logs the current lifecycle snapshot for a world.
- SOTS.Core.DumpListeners: lists registered lifecycle listeners.
- SOTS.Core.DumpSaveParticipants: lists registered save participants (if enabled).
- SOTS.Core.Health: prints a concise health report (settings + snapshot + counts).

## SPINE8: Automation tests (compile-only)
SOTS_Core includes minimal automation tests compiled only when WITH_AUTOMATION_TESTS is enabled.
These focus on dispatch gating, duplicate suppression, and map travel notification behavior without
requiring editor runtime.

## SPINE9: Versioning & config migration (log-only)
SOTS_Core defines semantic/version constants and a config schema version to support safe upgrades.
Migrations only normalize schema versions and log at Verbose (or Log if verbose logging is enabled).
Diagnostics also emit non-fatal warnings when configuration is likely misconfigured.

## SPINE10: Hardening + adoption guide
SOTS_Core includes hardening guardrails (safe unbinds, duplicate-fire warnings) and a documented
golden-path adoption checklist. See `Docs/SOTS_Core_Adoption_GoldenPath.md`.
