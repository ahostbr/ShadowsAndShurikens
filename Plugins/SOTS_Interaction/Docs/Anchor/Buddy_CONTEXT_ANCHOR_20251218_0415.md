[CONTEXT_ANCHOR]
ID: 20251218_0415 | Plugin: SOTS_Interaction | Pass/Topic: SPINE1_CoreLoop | Owner: Buddy
Scope: Core loop lock (driver API, hybrid data, tag/trace config)

DONE
- Added FSOTS_InteractionTraceConfig + scoring on context; candidate search now uses configurable shape/channel/distance with legacy fallbacks and fires candidate change when target or options change.
- Hybrid provider: component-first options (with defaults/meta/tags/LOS/range), interface fallback via BuildInteractionData; options gated via TagManager union helper EvaluateTagGates and cached per candidate.
- Driver exposes focus/score/options getters, TryInteract with verb, ForceRefreshFocus, and delegates OnFocusChanged/OnOptionsChanged/OnInteractRequested while forwarding UI intents only.
- Execution honors option overrides for range/LOS, allows interface-only targets, and reuses tag gating/results.
- Doc added: Docs/SOTS_Interaction_CoreLoop.md; Worklog recorded for SPINE1 core loop.

VERIFIED
- UI ban scan: DevTools/ad_hoc_regex_search for CreateWidget/AddToViewport/PushWidget/PopWidget returned no matches.

UNVERIFIED / ASSUMPTIONS
- Runtime selection/interaction flow not playtested; TraceConfig/editor serialization not validated.
- Legacy SearchDistance/Radius fallback assumed acceptable alongside new TraceConfig surface.

FILES TOUCHED
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionTypes.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractableComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionDriverComponent.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionSubsystem.h
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionSubsystem.cpp
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionDriverComponent.cpp
- Plugins/SOTS_Interaction/Docs/SOTS_Interaction_CoreLoop.md
- Plugins/SOTS_Interaction/Docs/Worklogs/SOTS_Interaction_SPINE1_CoreLoop_20251218_0410.md

NEXT (Ryan)
- Build/editor pass: confirm TraceConfig (channel/shape/radius/socket) editable and selection/focus updates fire with option changes.
- Validate interface-only interactables can be selected/executed; confirm option tag gating via TagManager union behaves as expected.
- Exercise driver delegates + TryInteract with multiple options; ensure UI router still receives intents only.
- Add/adjust any missing tags only if referenced at runtime.

ROLLBACK
- Revert touched SOTS_Interaction public/private files and docs listed above (or git revert commit) to undo SPINE1 core loop changes.
