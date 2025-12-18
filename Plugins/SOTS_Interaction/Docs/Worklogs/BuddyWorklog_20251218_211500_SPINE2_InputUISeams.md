# Worklog — SPINE2 Input/UI Seams (Driver)

## Goal
- Add SOTS_Input intent seam for interaction driver and expose prompt data without creating UI.

## What Changed
- Bound driver to SOTS_Input router intent delegate and added Blueprint callable intent handler/bind helper.
- Implemented prompt spec caching/emission with selection heuristics and change detection.
- Added doc outlining input/prompt seam expectations.

## Files Changed
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionDriverComponent.cpp
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionDriverComponent.h
- Plugins/SOTS_Interaction/Docs/SOTS_Interaction_InputAndUISeams.md

## Notes / Risks / Unknowns
- Selection sticks to previous index if still in range; does not re-evaluate if option payload changes at same index.
- Input intent consumption only on Triggered/Started; if handler emits other trigger events they are ignored.
- Prompt/UI behavior unverified in runtime/editor; relies on router auto-bind success.

## Verification
- Not built; no runtime/editor validation performed.

## Cleanup
- Binaries/ and Intermediate/ folders for SOTS_Interaction did not exist; no deletion needed.

## Follow-ups
- Runtime test: verify interact intent triggers option execution and prompt updates.
- Confirm prompt delegate consumers render desired UI and respect blocked options.
- Consider keeping selection when options reorder by matching OptionTag instead of index.
