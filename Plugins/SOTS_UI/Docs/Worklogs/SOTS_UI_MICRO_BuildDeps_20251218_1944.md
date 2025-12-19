# SOTS_UI MICRO BuildDeps (2025-12-18 19:44)

- Added `SOTS_Interaction` to `SOTS_UI.Build.cs` public dependencies so Interaction types resolve in UI router.
- Context: fixes missing `SOTS_InteractionTypes.h` include during editor build.
- No runtime logic changes; no build/run performed here.
