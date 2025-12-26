# SOTS_Core Adoption Golden Path

This guide is the no-risk path for adopting SOTS_Core without changing gameplay or UI behavior.
It assumes SOTS_Core is already in the project and enabled by default.

## Phase 0 (today): no Blueprint changes
Goal: confirm SOTS_Core is present and inert.

- Do not reparent any Blueprints yet.
- Keep settings bridges disabled (default).
- Use diagnostics commands:
  - `SOTS.Core.DumpSettings` to confirm default flags.
  - `SOTS.Core.Health` to confirm the subsystem is present.
  - `SOTS.Core.DumpLifecycle` (in an active world) to view the current snapshot.

Expected log strings:
- `SOTS_Core Settings:`
- `SOTS_Core Health Report:`
- `SOTS_Core Snapshot:`

## Phase 1 (safe): reparent-only steps
Goal: reparent CGF Blueprints to the SOTS_Core bases with no functional changes.

Reparent these Blueprints:
- BP_CGF_GameMode -> ASOTS_GameModeBase
- BP_CGF_GameState -> ASOTS_GameStateBase
- BP_CGF_PlayerState -> ASOTS_PlayerStateBase
- BP_CGF_PlayerController -> ASOTS_PlayerControllerBase
- BP_CGF_HUD -> ASOTS_HUDBase
- BP_CGF_Player -> ASOTS_PlayerCharacterBase

Notes:
- Only reparent; do not delete nodes or add logic.
- If any Blueprint has a custom parent chain, record it before reparenting.

## Verification checklist
Use the diagnostics commands and optional verbose logs to confirm lifecycle hooks are firing.

Commands:
1) `SOTS.Core.DumpSettings` (confirm expected flags)
2) `SOTS.Core.Health` (confirm snapshot + counts)
3) `SOTS.Core.DumpLifecycle` (confirm PC/Pawn/HUD pointers)

Optional (requires `bEnableVerboseCoreLogs=true`):
- Look for logs like `NotifyWorldStartPlay`, `NotifyPostLogin`, `NotifyPawnPossessed`, `NotifyHUDReady`.

## Rollback checklist
If anything feels off:
- Reparent Blueprints back to their previous parents (recorded in Phase 1).
- Do not delete SOTS_Core or its config; keep the plugin installed.
- Do not delete or rename any Blueprint assets during rollback.

## Settings enablement order (recommended)
1) Keep all bridges and dispatch disabled during Phase 0.
2) After reparenting and verification, enable:
   - `bEnableLifecycleListenerDispatch` if another plugin has registered a listener.
3) Optional (only if you need delegate bridges before reparenting):
   - `bEnableWorldDelegateBridge` for world-start events.
   - `bEnableMapTravelBridge` for travel events.
4) Always keep `bSuppressDuplicateNotifications` enabled if any bridge is enabled.

