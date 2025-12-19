# SOTS_GSM_SPINE3_ModifierStack_20251214_2017

Scope: handle-backed config/modifier stack, deterministic resolve order, dev-only introspection. Defaults preserve prior math/order.

Key changes
- Added FSOTS_GSM_Handle + ESOTS_GSM_RemoveResult for ownership tokens (SOTS_GlobalStealthTypes.h).
- Added deterministic config resolve: base config then highest-priority/most-recent override (Subsystem.cpp::ResolveEffectiveConfig).
- Modifiers now stored with handles/priority; applied in priority/created order (Subsystem.cpp::RecomputeGlobalScore).
- New handle-based APIs: AddStealthModifier/RemoveStealthModifierByHandle; PushStealthConfig/PopStealthConfig returning handles; legacy remove-by-source keeps working (Subsystem.h/.cpp).
- Introspection helpers + dev-only stack dumps (Subsystem.h/.cpp::GetActive*Handles, GetEffectiveTuningSummary, DebugDumpStackToLog).

Resolution order
1) BaseConfig (default asset or SetStealthConfig).
2) Config overrides sorted by Priority desc, CreatedTime desc; top override replaces base config.
3) Modifiers sorted by Priority desc, CreatedTime desc applied to light/visibility/global offsets/mults.
4) Existing tier/tags apply unchanged.

Debug controls
- bDebugLogStackOps / bDebugDumpEffectiveTuning (FSOTS_StealthScoringConfig) default false; DebugDumpStackToLog obeys shipping/test guard.

API surface (handles)
- AddStealthModifier(modifier, ownerTag, priority)->handle; RemoveStealthModifierByHandle(handle, requesterTag); RemoveStealthModifierBySource returns count.
- PushStealthConfig(asset, priority, ownerTag)->handle; PopStealthConfig(handle, requesterTag) with kind/owner validation.
- Introspection: GetActiveModifierHandles, GetActiveConfigHandles, GetEffectiveTuningSummary, DebugDumpStackToLog.

Files touched
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthTypes.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp

Cleanup
- Plugins/SOTS_GlobalStealthManager/Binaries present after cleanup: False
- Plugins/SOTS_GlobalStealthManager/Intermediate present after cleanup: False
