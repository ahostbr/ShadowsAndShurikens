# SOTS_FX_Plugin — SPINE 1 (Registry Validation & Deterministic Load)

## What changed
- Established a deterministic library-based registry: libraries are loaded once on subsystem init and cached in `RegisteredLibraryDefinitions`; duplicate tags keep the first definition.
- Added dev-only validation toggles (default false) for duplicates, missing assets, and trigger-before-ready warnings; validation runs on init when enabled.
- Added registry-ready gating to all trigger/play paths to avoid pre-init requests.

## New config flags (defaults)
- `bValidateFXRegistryOnInit=false`
- `bWarnOnDuplicateFXTags=false`
- `bWarnOnMissingFXAssets=false`
- `bWarnOnTriggerBeforeRegistryReady=false`

## Evidence pointers
- Library load/cache: `BuildRegistryFromLibraries` in `Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp`.
- Validation helpers: `ValidateLibraryDefinitions` / `ValidateCueDefinition` (same file; dev-only guarded).
- Registry gating: `bRegistryReady` + `EnsureRegistryReady` guard used by `PlayCueByTag`, `PlayCueDefinition`, `RequestFXCue`, `BroadcastResolvedFX` (same file).
- Config toggles and cached map: `bValidateFXRegistryOnInit`, `bWarn...`, `RegisteredLibraryDefinitions` (`Public/SOTS_FXManagerSubsystem.h`).

## Duplicate / missing policy
- Duplicate tags: first definition wins; optional warning when enabled.
- Missing assets: warns (dev-only) if both Niagara and Sound are unset.

## Behavior impact
- Defaults preserve prior behavior; validation and warnings are opt-in dev-only. No runtime auto-scanning added.
