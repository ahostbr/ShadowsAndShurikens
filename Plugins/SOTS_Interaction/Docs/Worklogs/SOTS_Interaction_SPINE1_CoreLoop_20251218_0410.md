# Worklog — SOTS_Interaction SPINE1 CoreLoop (Buddy)

## Goal
- Lock SOTS_Interaction core loop surfaces (brain/driver/data) without UI creation; hybrid component-first provider, TagManager union gating, and BP-friendly driver API.

## What changed
- Added trace/config surface (`FSOTS_InteractionTraceConfig`) and scored context to subsystem; selection now uses configurable shape/channel/distance, applies TagManager union gating to options, and caches options/score for candidate change notifications.
- Enabled interface-only interactables (component-first priority, interface fallback) with hybrid data packaging (`FSOTS_InteractionData`) and expanded option payload (tags, LOS/range overrides, priority).
- Driver exposes BP callable focus/options/score getters, TryInteract with chosen verb, force refresh, and new delegates for focus/options/interact while forwarding UI intents only (no widget creation).
- Execution respects option overrides for LOS/range, reuses tag gating results, and honors global LOS toggle.
- Docs added for core loop contract; UI ban verified via regex scan (no CreateWidget/AddToViewport/PushWidget/PopWidget matches).

## Files changed
- Source/SOTS_Interaction/Public/SOTS_InteractionTypes.h
- Source/SOTS_Interaction/Public/SOTS_InteractableComponent.h
- Source/SOTS_Interaction/Public/SOTS_InteractionDriverComponent.h
- Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h
- Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Source/SOTS_Interaction/Private/SOTS_InteractionDriverComponent.cpp
- Docs/SOTS_Interaction_CoreLoop.md

## Notes / Risks / Unknowns
- Option-level distance/LOS overrides are enforced at execution; selection still assumes component/global LOS/distance and requires an available (unblocked) option. If per-option LOS differs from component, selection may still filter via component LOS.
- TraceConfig currently uses its own values with legacy SearchDistance/SearchRadius as fallback; if legacy properties are edited without TraceConfig, behavior may diverge (TraceConfig should be treated as the primary surface).
- TagManager availability still fails open (per prior behavior).
- No runtime verification/build performed.
- Potential additional UI intent wiring/input binding refinements are deferred to SPINE2.

## Verification
- Static checks only: ad_hoc_regex_search confirmed no UI creation calls in SOTS_Interaction. No builds or runtime tests run.

## Cleanup
- Pending: delete Plugins/SOTS_Interaction/Binaries and Plugins/SOTS_Interaction/Intermediate after finishing review.

## Follow-ups / Next steps
- SPINE2: UI intent/router wiring, input binding specifics, and any required schema tags. Confirm TraceConfig serialization/use in editor. Validate option gating/selection in runtime once builds are available.
