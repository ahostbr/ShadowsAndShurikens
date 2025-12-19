# SOTS_Interaction FIX InputDeps (2025-12-18 20:08)

- Added `SOTS_Input` as a public module dependency and moved `EnhancedInput` to private deps in `SOTS_Interaction.Build.cs` so public headers can include SOTS_Input types.
- Synced plugin deps to include `SOTS_Input`, `EnhancedInput`, and `OmniTrace` in `SOTS_Interaction.uplugin`.
- No runtime testing performed.
