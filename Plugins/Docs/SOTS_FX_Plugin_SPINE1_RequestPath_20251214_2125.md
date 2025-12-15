# SOTS_FX_Plugin SPINE1 - Request Path (2025-12-14 21:25)

## What changed
- Added Blueprint-visible request outcome types `ESOTS_FXRequestResult` and `FSOTS_FXRequestReport` for lightweight FX cue reporting.
- Implemented `RequestFXCueWithReport` and routed the legacy `RequestFXCue`/Trigger entrypoints through a unified `ProcessFXRequest` that fills reports, logs dev-only failures, and avoids silent drops.
- Centralized tag resolution/policy checks in `TryResolveCue` + `EvaluatePolicy`, keeping library order intact and returning explicit status codes.

## Current public entrypoints
- Report-capable: `TriggerFXByTagWithReport` (legacy struct) and new `RequestFXCueWithReport` (report struct).
- Fire-and-forget: `RequestFXCue`, `TriggerFXByTag`, `TriggerAttachedFXByTag` now internally use the reporting path (failures still logged when dev logging is enabled).

## BEP usage
- `rg "RequestFXCue" E:/SAS/ShadowsAndShurikens/BEP_EXPORTS/SOTS --iglob "*.txt"` -> no hits (no BEP callers today).
- BEP FX usage observed is asset-direct Niagara/Sound/Camera shake wiring (e.g., `BEP_EXPORTS/SOTS/2025-12-12_09-24-57/Blueprints/BP_SOTS_VFX_SPAWNER/Flow.txt`).

## Evidence pointers
- Result enum/report types: `Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h:17-55`.
- Request entrypoints + helper declarations: `Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h:87-166`.
- Request implementation with report: `Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp:236-264`.
- Unified processing + failure logging: `Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp:395-526`.
- Legacy conversion helper: `Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp:544-574`.
- Policy + resolver: `Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp:588-604`.

## Notes
- Policy gating currently pass-through (no per-cue metadata yet); failures now return explicit reasons (InvalidWorld/InvalidParams/NotFound) instead of silent drops.
- Dev logging remains gated by `bLogFXRequestFailures` and shipping/test guards.
- Plugin Binaries/Intermediate were absent (checked and cleaned if present).
