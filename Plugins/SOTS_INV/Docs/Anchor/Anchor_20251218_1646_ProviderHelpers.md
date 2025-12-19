[CONTEXT_ANCHOR]
ID: 20251218_1646 | Plugin: SOTS_INV | Pass/Topic: ProviderHelpers | Owner: Buddy
Scope: Added blueprint-facing inventory provider resolution helpers (world context and controller-first ordering).

DONE
- Added BlueprintPure helpers GetInventoryProviderFromWorldContext and ResolveInventoryProviderFromControllerFirst in SOTS_InventoryFacadeLibrary.
- Routed internal provider resolution through the new helpers to keep controller-first, pawn-second ordering.

VERIFIED
- None (no build or runtime executed).

UNVERIFIED / ASSUMPTIONS
- Pickup/UI flows are expected to keep resolving providers without InvSP dependencies; not runtime-validated.

FILES TOUCHED
- Plugins/SOTS_INV/Source/SOTS_INV/Public/SOTS_InventoryFacadeLibrary.h
- Plugins/SOTS_INV/Source/SOTS_INV/Private/SOTS_InventoryFacadeLibrary.cpp

NEXT (Ryan)
- Open the project and confirm Blueprints can call the new provider helpers (returns non-null for expected contexts).
- Validate pickup flow still resolves providers correctly when inventory components live on controllers/pawns.
- Run a minimal editor session to ensure no load errors from the new facade signatures.
- No plugin build artifacts present; proceed with standard build when convenient.

ROLLBACK
- Revert changes in SOTS_InventoryFacadeLibrary.h and SOTS_InventoryFacadeLibrary.cpp to the previous revision.
