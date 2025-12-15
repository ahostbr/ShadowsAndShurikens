# DragonStealthComponent_Expose3Floats (2025-12-14 20:46)

## Summary
- Exposed normalized (0..1) telemetry floats for AI perception, light level, and overall detection on `USOTS_DragonStealthComponent` as BlueprintReadOnly properties with clamp metadata.
- Synced the new properties inside `HandleStealthStateChanged` using clamped values from `FSOTS_PlayerStealthState` fields (`AISuspicion01`, `LightLevel01`, `GlobalStealthScore01`).
- Added Blueprint getters for AI perception and overall detection; light getter now returns the clamped property.

## Evidence
- Properties and getters: Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_DragonStealthComponent.h:25,37-40,46-52
- Clamped updates from stealth state: Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_DragonStealthComponent.cpp:31-36

## Notes
- Source fields: `FSOTS_PlayerStealthState::AISuspicion01`, `FSOTS_PlayerStealthState::LightLevel01`, `FSOTS_PlayerStealthState::GlobalStealthScore01`.
- Defaults remain zeroed until GSM broadcasts a state change.
