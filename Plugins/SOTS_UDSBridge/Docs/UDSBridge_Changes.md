# UDS Bridge Changes

## What changed
- Added Blueprint library entry points to query bridge state and trigger a refresh/apply cycle (see SOTS_UDSBridgeBlueprintLib and subsystem wiring).
- Introduced project-level developer settings (`SOTS UDS Bridge`) to select a default `USOTS_UDSBridgeConfig` asset for subsystem startup.
- Subsystem now exposes explicit snapshot/refresh helpers, reuses a shared cache-to-state builder, and respects Global Stealth Manager sun pushes and DLWE policy application.
- Documentation in this folder now includes this change log alongside UDSBridge_Notes.md.

## How to use
- In Project Settings → Game → **SOTS UDS Bridge**, assign `DefaultBridgeConfig` to your bridge config asset.
- From Blueprints, call `GetUDSBridgeState` to read cached weather/GSM status and `ForceUDSBridgeRefresh` to rescan and reapply policy on demand.

## Notes
- No build rules were modified; the subsystem still depends on Core/CoreUObject/Engine with private SOTS_GlobalStealthManager.
- Generated artifacts (Binaries/Intermediate) were cleaned after these edits.
