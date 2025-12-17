# Buddy Worklog — ShaderWarmup dependency API & cleanup

- **Goal**: Fix SOTS_UI compile errors on UE 5.7p (AssetRegistry API changes, missing UE_UNUSED macro, and private router pop access).
- **Changes**:
  - Swapped AssetRegistry dependency query to the new API (`UE::AssetRegistry::EDependencyCategory::Package` + `FDependencyQuery`).
  - Replaced `UE_UNUSED` usages with `(void)` casts.
  - Exposed `PopWidgetById` as a public BlueprintCallable on `USOTS_UIRouterSubsystem` so shader warmup teardown can pop its widget legally.
- **Files**:
  - Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_ShaderWarmupSubsystem.cpp
  - Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- **Notes/Risks**: Router pop exposure is additive; behavior unchanged aside from API access. Asset registry query now uses package category with default query.
- **Verification**: UNVERIFIED (no build/run executed).
- **Cleanup**: Deleted Plugins/SOTS_UI/Binaries and Intermediate.
- **Follow-ups**: Ryan to rebuild SOTS_UI on UE 5.7p; if further AssetRegistry API adjustments are needed (e.g., options flags), report exact errors.
