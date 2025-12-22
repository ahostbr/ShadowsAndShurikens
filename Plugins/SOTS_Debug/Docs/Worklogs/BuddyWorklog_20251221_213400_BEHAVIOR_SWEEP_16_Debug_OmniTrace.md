# Buddy Worklog — BEHAVIOR_SWEEP_16 Debug/OmniTrace (2025-12-21 21:34)

- Replaced `LogTemp` usage in SuiteDebug subsystem with a stable `LogSOTSSuiteDebug` category to align with logging locks.
- No input wiring changes yet; one-button debug trigger via SOTS_Input remains to be verified/wired.
- OmniTrace BP debug draw entrypoints untouched; still compiled out in Shipping/Test.
- No builds or runtime execution per instructions.
- Cleanup: deleted Plugins/SOTS_Debug/Binaries and Plugins/SOTS_Debug/Intermediate.
