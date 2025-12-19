# SOTS_INV FIX BreakPluginCycle (2025-12-18 20:48)

- Removed SOTS_INV -> SOTS_UI plugin/module dependency (uplugin entry dropped; Build.cs no longer lists SOTS_UI).
- Reworked UI calls in `SOTS_InventoryFacadeLibrary.cpp` and `SOTS_InventoryBridgeSubsystem.cpp` to resolve the UI router dynamically via soft class path and reflection (no compile-time dependency) while keeping UI notifications/requests.
- Cleaned helper usage; SOTS_INV now compiles without referencing SOTS_UI headers.
- No runtime testing performed.
