# SOTS_GSM_CompletenessPass_PLAN_20251214_1936

Phase 0 read-only sweep. No code changes performed.

Evidence - ingestion and tier compute
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::ReportStealthSample`: clamps sample fields, computes score from light/LOS/distance/noise/cover, overwrites `LastSample`, immediate tier mapping, no history.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::UpdateFromPlayer` and `RecomputeGlobalScore`: clamps player state, blends LocalVisibility vs AISuspicion, applies modifiers, maps tiers, broadcasts, logs warning each call.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::ReportAISuspicion`: per-guard suspicion map, max aggregate, removes invalid weak ptrs only.
- `Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp::UpdatePerception` (around suspicion fold): sends normalized suspicion via `ReportAISuspicion`, detection state uses `ReportEnemyDetectionEvent` in `ReportAlertStateChanged`.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthDebugLibrary.cpp::RunSOTS_GSM_DebugSample`: builds sample ad hoc for testing.
- `Plugins/LightProbePlugin/Source/LightProbePlugin/Private/LightLevelProbeComponent.cpp::UpdateLightLevel`: feeds normalized light into `USOTS_PlayerStealthComponent::SetLightLevel` on timer.

Evidence - modifiers and config
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::AddStealthModifier` and `RemoveStealthModifierBySource`: TArray keyed by `SourceId`, replace on duplicate, apply offsets/mults during `RecomputeGlobalScore`.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::PushStealthConfig` / `PopStealthConfig`: stack of assets without ownership tokens; logs on empty pop; ActiveConfig copied from asset.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_StealthConfigDataAsset.h`: config asset wraps `FSOTS_StealthScoringConfig` thresholds/weights, no versioning.

Evidence - outputs/tagging/events
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::UpdateGameplayTags`: applies SOTS.Stealth.* tier and light tags via TagLibrary on player actor.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::SyncPlayerStealthComponent`: mirrors state into player component, sets cover/shadow/detected flags.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h`: delegates `OnStealthLevelChanged`, `OnPlayerDetectionStateChanged`, `OnStealthStateChanged`.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthTagLibrary.cpp`: BP helpers fallback to `Player.Stealth.*` tags when GSM absent.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_PlayerStealthComponent.cpp::SetStealthTier/SetIsInCover/SetIsInShadow/SetDetected`: pushes `SAS.Stealth.*` tags via TagManager subsystem.
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_DragonStealthComponent.cpp`: subscribes to `OnStealthStateChanged`.

Evidence - profile integration
- `Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::BuildProfileData` zeros FSOTS_GSMProfileData; `ApplyProfileData` no-ops.
- Profile struct defined in `Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileTypes.h`.

Gaps / missing behaviors (proof above)
- Ingestion is per-call only: `ReportStealthSample` uses single sample, no time window or ordering; `LastSample` stored but unused elsewhere.
- No throttling/expiry for AI suspicion map: `ReportAISuspicion` keeps last value per guard until GC, no decay beyond caller.
- Movement/cover/noise inputs are not sourced: `SetMovementNoise` and `SetCoverExposure` only defined in `SOTS_PlayerStealthComponent.cpp` with no callers outside plugin (rg shows no external use).
- Tier stability: `EvaluateStealthLevelFromScore` and tier mapping in `RecomputeGlobalScore` are instantaneous thresholds; no hysteresis/smoothing; detection event sets level/score but skips `UpdateGameplayTags`/`OnStealthStateChanged` sync.
- Modifier stack safety: modifiers keyed only by `SourceId` (no ownership handles or durations); config stack push/pop lacks owner tokens or scope guards (any caller can pop entire stack).
- Output contract drift: GSM tags use `SOTS.Stealth.*`, player component uses `SAS.Stealth.*`, tag library fallbacks look for `Player.Stealth.*`, so tier tags are not single-source and may diverge.
- Delegate payloads are minimal: `OnStealthLevelChanged` carries levels/score only; no reason/context; `OnStealthStateChanged` not fired on `ReportEnemyDetectionEvent`.
- Profile persistence unspecified: Build/Apply effectively stateless (defaults + no-op), no versioning or validation.

Spine (next actions)
- SPINE_1: Harden input ingestion (validation, clamping, sample cadence, drop logging) across player samples and AI suspicion.
- SPINE_2: Tier computation stability (smoothing/hysteresis/decay, detection integration, explicit transition events).
- SPINE_3: Modifier/config stack ownership (tokens/handles, leak-proof push/pop, duration support, diagnostics).
- SPINE_4: Output contract (single tag namespace, consistent delegates payload, BP helper alignment).
- SPINE_5: Profile integration (snapshot policy/versioning, deterministic apply/clear if stateless).
- SPINE_6: Shipping safety (gate debug logs/draws by config and build, remove dead code/TODOs, clean binaries/intermediate post-edit).
