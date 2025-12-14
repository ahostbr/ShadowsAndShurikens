# SOTS BodyDrag — SPINE 3 (Scaffold, Types, ConfigDA)

## Summary
- Added core BodyDrag types (state/body enums, montage set, config struct) for KO vs Dead handling.
- Introduced a DataAsset (USOTS_BodyDragConfigDA) to author BodyDrag config in Blueprint.
- Registered default gameplay tags for drag state/targets and interaction options.

## Types
- `ESOTS_BodyDragState` (None/Entering/Dragging/Exiting) — tracks drag lifecycle.
- `ESOTS_BodyDragBodyType` (KnockedOut/Dead) — distinguishes KO vs Dead bodies.
- `FSOTS_BodyDragMontageSet` — Start/Stop drag montages per body type.
- `FSOTS_BodyDragConfig` — KO/Dead montage sets, attach socket, drag speed, physics re-enable flags, tag names.
- `USOTS_BodyDragConfigDA` — DataAsset exposing `FSOTS_BodyDragConfig` to Blueprint.

## Tags added (DefaultGameplayTags.ini)
- SAS.BodyDrag.State.Dragging
- SAS.BodyDrag.Target.Dead
- SAS.BodyDrag.Target.KO
- SAS.Interaction.Option.BodyDrag.Start
- SAS.Interaction.Option.BodyDrag.Drop

## Notes
- Four montages are expected: KO start/stop and Dead start/stop for tailored animations.
- Plugin-only; no game module dependencies or CharacterBase assumptions.
- No UI creation; no build executed.
