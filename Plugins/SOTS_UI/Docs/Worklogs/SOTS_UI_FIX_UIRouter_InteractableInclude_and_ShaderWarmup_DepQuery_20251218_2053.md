# SOTS_UI FIX UIRouter InteractableInclude and ShaderWarmup DepQuery (2025-12-18 20:53)

- Added `SOTS_InteractableComponent.h` include in `Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp` to satisfy the compile-time definition when accessing pickup metadata (fixes undefined type error).
- Updated shader warmup dependency query to use UE5.7 `EDependencyProperty::None` for Required/Excluded in `Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp` (fixes EDependencyQuery -> EDependencyProperty mismatch).
- Cleanup: removed `Plugins/SOTS_UI/Binaries` and `Plugins/SOTS_UI/Intermediate`.
- No runtime testing performed.
