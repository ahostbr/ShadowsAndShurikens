# SOTS_Input Attachment and Discovery

Scope: canonical ways to resolve or attach the input router/buffer for Blueprints and other plugins.

## Attachment target
- Attach `USOTS_InputRouterComponent` and `USOTS_InputBufferComponent` to the local player controller (not the pawn). This keeps controller-owned Enhanced Input and subsystems aligned.
- Avoid duplicating routers/buffers across pawns/possessions; one instance on the player controller should serve the active pawn.

## PlayerController resolution order
1) If `ContextActor` **is** a `PlayerController`, use it.
2) If `ContextActor` is a `Pawn`, try its controller → cast to `PlayerController`.
3) If `ContextActor` has an instigator controller, cast to `PlayerController`.
4) Otherwise, use `WorldContextObject` → `World->GetFirstPlayerController()`.
5) Final fallback: `ContextActor->GetWorld()->GetFirstPlayerController()`.

## Blueprint helpers (USOTS_InputBlueprintLibrary)
- `GetRouterForPlayerController(WorldContextObject, ContextActor)`: Resolve an existing router using the order above (no creation).
- `EnsureRouterOnPlayerController(WorldContextObject, ContextActor)`: Resolve PC, return existing router if present, otherwise create/register a transient router component on the PC.
- `EnsureBufferOnPlayerController(WorldContextObject, ContextActor)`: Same pattern for the buffer component.
- Legacy helpers remain (`GetRouterFromActor`, `PushLayerTag`, `PopLayerTag`, `OpenBuffer`, `CloseBuffer`); prefer the PlayerController-aware helpers for new logic.

## Creation behavior
- Ensure helpers add instance components to the player controller, mark them transient, call `OnComponentCreated`, and register them so `OnRegister`/auto-refresh runs immediately.
- Components are not saved or duplicated; repeated calls return the existing instance on the controller.

## Recommended usage
- On UI or gameplay entry points, call `EnsureRouterOnPlayerController` and `EnsureBufferOnPlayerController` with a known world context (HUD, GameInstance, or the pawn). Keep the returned router reference for push/pop and buffering calls.
- For tag-driven layer changes: resolve/ensure router → `PushLayerTag`/`PopLayerTag` as before.
- For buffering: resolve/ensure buffer via the helper, then use `OpenBuffer`/`CloseBuffer` on the router.

## Notes
- Helpers log a warning if no `PlayerController` is found; they do not attempt to create components elsewhere.
- No editor/build verification performed in this pass (runtime-only reasoning).
