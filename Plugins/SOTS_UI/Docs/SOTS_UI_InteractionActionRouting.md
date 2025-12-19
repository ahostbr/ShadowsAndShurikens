# SOTS_UI — Interaction Action Routing (Bridge Pass 1)

## Summary
- USOTS_UIRouterSubsystem now subscribes to SOTS_Interaction OnInteractionActionRequested.
- Verb Interaction.Verb.Pickup routes to SOTS_INV::RequestPickup via inventory facade.
- Execute / DragStart / DragStop are stubbed (logged in dev builds) pending later routing.

## Flow
1) Router initialization ensures a single binding to USOTS_InteractionSubsystem.
2) OnInteractionActionRequested(Request):
   - If verb is Pickup, build FSOTS_InvPickupRequest and call USOTS_InventoryFacadeLibrary::RequestPickup.
   - If ItemTag is missing, the router will attempt a best-effort fallback by reading `PickupItemTag`/`PickupQuantity` from the target actor's `USOTS_InteractableComponent` before skipping; if still missing, the request is skipped with a dev-only log (no other coupling yet).
   - Other verbs: no-op except a dev log in non-shipping.
3) UI notifications remain handled by SOTS_INV (SPINE_3) via RequestInvSP_*; router does not create UI here.

## Defaults / Payload
- Quantity falls back to 1 if Request.Quantity <= 0.
- ContextTags from the request are forwarded; adds Context.Interaction tag when available.
- Pickup consumes world actor and notifies UI according to SOTS_INV defaults (flags remain true).
