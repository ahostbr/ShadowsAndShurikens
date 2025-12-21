[CONTEXT_ANCHOR]
ID: 20251221_231900 | Plugin: SOTS_GAS_Plugin | Pass/Topic: AbilityFX -> FXManager | Owner: Buddy
Scope: Route ability FX triggers through the central FX manager subsystem.

DONE
- Replaced the stubbed `TriggerAbilityFX` implementation with a world/context-safe call into `USOTS_FXManagerSubsystem::TriggerFXByTag` and kept the verbose log under `LogSOTSGAS` behind the existing shipping/test guard.
- Added the FX manager and plugin log headers so the subsystem can discover the manager and report on missing contexts.

VERIFIED
- None (no build or runtime execution was allowed).

UNVERIFIED / ASSUMPTIONS
- Assumes the FX manager is registered and reachable via `USOTS_FXManagerSubsystem::Get`; if it is not, the subsystem now logs a warning and skips the FX job.
- Behavior change has not been validated in editor or runtime; effectiveness of the new call path is unverified.

FILES TOUCHED
- Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityFXSubsystem.cpp

NEXT (Ryan)
- Confirm ability execution still produces the expected FX cues now that the subsystem routes through the FX manager (check logs/effects in-editor or in-game).

ROLLBACK
- Revert Plugins/SOTS_GAS_Plugin/Source/SOTS_GAS_Plugin/Private/Subsystems/SOTS_AbilityFXSubsystem.cpp
