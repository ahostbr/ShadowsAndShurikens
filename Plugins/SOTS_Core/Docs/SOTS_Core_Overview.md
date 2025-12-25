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
