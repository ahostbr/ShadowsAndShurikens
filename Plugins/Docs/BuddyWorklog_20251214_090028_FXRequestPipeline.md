# Buddy Worklog — FX Request Pipeline

## Goal
Unify tag-driven FX requests behind a single pipeline with explicit result payloads, keep backward-compatible entry points, and add dev-only failure logging (SOTS_FX_Plugin only).

## Changes
- Added `FSOTS_FXRequest`, `ESOTS_FXRequestStatus`, and `FSOTS_FXRequestResult` to carry request inputs and results explicitly.
- Routed subsystem triggers (world and attached) and `RequestFXCue` through a new `ProcessFXRequest` path that validates tags/registry/attachments, resolves definitions, and broadcasts with requested scale.
- Added a report-returning trigger (`TriggerFXByTagWithReport`) plus Blueprint library helper (`TriggerFXWithReport`) returning structured results.
- Added dev-only failure logging toggle `bLogFXRequestFailures` with guarded logging for invalid tag, registry not ready, missing definition/attachment.
- Broadcast now includes requested scale and preserves resolved assets in the result payload.

## Files Touched
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTSFXTypes.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXBlueprintLibrary.h
- Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXBlueprintLibrary.cpp

## Verification
- Not run (per instructions: no builds/tests).

## Cleanup
- Deleted plugin-local Binaries/ and Intermediate/ after edits.

## Follow-ups
- Consider Blueprint-facing convenience wrapper to auto-populate location/rotation from actor if omitted.
- Add automated tests or a debug command to simulate various failure codes once allowed.
