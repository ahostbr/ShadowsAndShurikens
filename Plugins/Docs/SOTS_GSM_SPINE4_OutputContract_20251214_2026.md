# SOTS_GSM_SPINE4_OutputContract_20251214_2026

Scope: centralize GSM stealth tags, add reasoned delegates, BP helpers, drift repair (opt).

Tag schema and mapping
- Tier->Tag mapping uses existing SOTS.Stealth.State.* tags via GetTierTag (SOTS_GlobalStealthManagerSubsystem.cpp:858-882).
- Light/Dark tags still applied via TagManager in UpdateGameplayTags (same mapping, now only light/shadow).

Central tag application
- ApplyStealthStateTags removes old tier tags, applies current, caches last applied, optional drift repair (SOTS_GlobalStealthManagerSubsystem.cpp:884-925).
- UpdateGameplayTags now only handles light/dark tags using TagManager (SOTS_GlobalStealthManagerSubsystem.cpp:203-235).

Delegates + payloads
- OnStealthScoreChanged now emits FSOTS_StealthScoreChange with Old/New score, ReasonTag, Timestamp (Subsystem.h:13-16,66-69; Subsystem.cpp:360-374).
- OnStealthTierChanged uses FSOTS_StealthTierTransition now including ReasonTag + ActiveModifierOwners (Types.h:133-189; Subsystem.cpp:347-374).
- Reason tags sourced from inputs: light (SAS.Stealth.Driver.Light), LOS detection (SAS.Stealth.Driver.LineOfSight), AI suspicion (SAS.AI) (Subsystem.cpp:62-83,155-177,189-212).

BP helper surface
- USOTS_GSM_BlueprintLibrary exposes score/tier/tier tag/tier check/last reason (Public/Private SOTS_GSM_BlueprintLibrary.{h,cpp}).

Config/debug knobs
- bRepairStealthTagsIfDriftDetected (default false), bDebugLogStackOps, bDebugDumpEffectiveTuning in FSOTS_StealthScoringConfig (Types.h:145-185).

Files changed
- Config/types: Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthTypes.h
- Subsystem: Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/Private/SOTS_GlobalStealthManagerSubsystem.*
- BP helpers: Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/Private/SOTS_GSM_BlueprintLibrary.*

Cleanup
- Plugins/SOTS_GlobalStealthManager/Binaries present after cleanup: False
- Plugins/SOTS_GlobalStealthManager/Intermediate present after cleanup: False
