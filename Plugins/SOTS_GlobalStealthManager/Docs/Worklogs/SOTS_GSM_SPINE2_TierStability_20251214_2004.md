# SOTS_GSM_SPINE2_TierStability_20251214_2004

Scope: Tier/score stability (smoothing, hysteresis, decay) with default behavior preserved.

Baselines (pre-change)
- Tier/score computed in RecomputeGlobalScore (Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp).
- Thresholds from FSOTS_StealthScoringConfig (SOTS_GlobalStealthTypes.h).
- Delegates: OnStealthLevelChanged only.

New tuning
- FSOTS_StealthTierTuning defaults: hysteresis OFF (HysteresisPadding=0), smoothing OFF (ScoreSmoothingHalfLifeSeconds=0), min tier dwell OFF, calm decay OFF (SOTS_GlobalStealthTypes.h:130-165).
- Added FSOTS_StealthTierTransition struct for transition payload (SOTS_GlobalStealthTypes.h:167-189).

Logic changes
- RecomputeGlobalScore now supports optional calm decay, score smoothing, hysteresis, and min tier dwell; defaults keep prior behavior (Subsystem.cpp:307-385).
- Tier selection via hysteresis-aware helper; uses TierTuning thresholds; level mapping unchanged (Subsystem.cpp:333-368).
- New delegates: OnStealthScoreChanged, OnStealthTierChanged with FSOTS_StealthTierTransition (Subsystem.h:13-16,57-64; Subsystem.cpp:375-384).

Defaults preserved
- All new toggles default false/zero so baseline tiering matches prior thresholds (Hidden<0.2<Cautious<0.5<Danger<0.8<Compromised).

Files touched
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthTypes.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthManagerSubsystem.h
- Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp

Cleanup
- Plugins/SOTS_GlobalStealthManager/Binaries removed if existed; Plugins/SOTS_GlobalStealthManager/Intermediate removed if existed.
