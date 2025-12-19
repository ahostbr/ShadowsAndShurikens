# Worklog — SOTS_UI BRIDGE Interaction Action Routing 1 (2025-12-18 23:56)

## Goal
Consume SOTS_Interaction action requests and route Pickup to SOTS_INV without new UI creation; stub other verbs.

## Changes
- Router now binds once to SOTS_InteractionSubsystem::OnInteractionActionRequested during initialization.
- Added handler to route Interaction.Verb.Pickup into USOTS_InventoryFacadeLibrary::RequestPickup using FSOTS_InvPickupRequest.
- Quantity falls back to 1; ContextTags forwarded with optional Context.Interaction tag. If ItemTag missing, request is skipped with a dev log (no hard coupling).
- Execute/DragStart/DragStop left as stubs with dev-only log.
- Documented flow in SOTS_UI_InteractionActionRouting.md.

## Files Touched
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp
- Plugins/SOTS_UI/Docs/SOTS_UI_InteractionActionRouting.md

## Notes / Risks / Unknowns
- ItemTag fallback not implemented (no pickup metadata source yet); requests without ItemTag are ignored. Future work: surface pickup metadata from interaction side.
- Assumes canonical verb tags exist (Interaction.Verb.*) from SOTS_Interaction pass.

## Verification
- No build or runtime tests executed (per instructions).

## Cleanup
- Deleted Plugins/SOTS_UI/Binaries and Plugins/SOTS_UI/Intermediate.
