# SOTS_GSM_SPINE5_ProfilePolicy_20251214_2033

Policy: STATELESS (GSM resets on init/profile load; no snapshot persisted).

Evidence
- ResetStealthState(ESOTS_GSM_ResetReason) clears score/tier/modifiers/config overrides and reapplies tier tags (Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp:284-323).
- ApplyProfileData calls ResetStealthState with ProfileLoaded reason (Subsystem.cpp:270-273).
- Initialize also calls ResetStealthState (Subsystem.cpp:31-38).
- Debug reset logging flag added: bDebugLogStealthResets (default false) (SOTS_GlobalStealthTypes.h:149-187).

State preserved/cleared
- Preserves BaseConfig/DefaultConfigAsset; clears modifier/config overrides, score/tier, last reason, smoothed score, tag caches.

Hooks
- Init and profile apply invoke reset; mission/level callers should invoke ResetStealthState when appropriate.

Files changed
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthTypes.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/Private/SOTS_GlobalStealthManagerSubsystem.*
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/Private/SOTS_GSM_BlueprintLibrary.*

Cleanup
- Plugins/SOTS_GlobalStealthManager/Binaries present after cleanup: False
- Plugins/SOTS_GlobalStealthManager/Intermediate present after cleanup: False
