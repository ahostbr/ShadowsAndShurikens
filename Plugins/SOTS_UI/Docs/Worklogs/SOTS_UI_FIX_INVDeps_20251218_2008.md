# SOTS_UI FIX INVDeps (2025-12-18 20:08)

- Added `SOTS_INV` to module private deps so UI router can include `SOTS_InventoryFacadeLibrary.h`; kept `SOTS_Interaction` in public deps.
- Updated plugin deps to include `SOTS_Interaction`, `SOTS_INV`, and `Niagara` alongside existing inputs to satisfy build/package dependency warnings.
- Adjusted shader warmup fallback to use UE5.7-compatible `FDependencyQuery` (no WithPackageFlags) in `SOTS_ShaderWarmupSubsystem.cpp`.
- No runtime testing performed.
