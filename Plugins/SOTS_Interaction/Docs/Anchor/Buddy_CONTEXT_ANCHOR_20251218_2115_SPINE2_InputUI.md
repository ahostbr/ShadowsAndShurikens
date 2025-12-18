[CONTEXT_ANCHOR]
ID: 20251218_2115 | Plugin: SOTS_Interaction | Pass/Topic: SPINE2_InputUI | Owner: Buddy
Scope: Interaction driver input intent + prompt surface wiring

DONE
- Driver binds to SOTS_Input router `OnInputIntent` and consumes `Input.Intent.Gameplay.Interact` via `HandleRouterIntentEvent`/`HandleInputIntentTag`; added BP callable `BindToInputRouter`.
- Prompt caching added: `FSOTS_InteractionPromptSpec` + `OnInteractionPromptChanged` delegate; selection heuristic picks highest-priority unblocked option else best available; prompt refresh on focus/options/selection changes and after interactions.
- Added docs: `Docs/SOTS_Interaction_InputAndUISeams.md` describing intent tag, prompt surface, router expectations.

VERIFIED
- None (no build/runtime; only code review-level changes).

UNVERIFIED / ASSUMPTIONS
- Router auto-bind succeeds for owner/controller; trigger events other than Triggered/Started intentionally ignored.
- Selection persists by index when option payload changes but index stays valid.

FILES TOUCHED
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Private/SOTS_InteractionDriverComponent.cpp
- Plugins/SOTS_Interaction/Source/SOTS_Interaction/Public/SOTS_InteractionDriverComponent.h
- Plugins/SOTS_Interaction/Docs/SOTS_Interaction_InputAndUISeams.md
- Plugins/SOTS_Interaction/Docs/Worklogs/BuddyWorklog_20251218_211500_SPINE2_InputUISeams.md

NEXT (Ryan)
- Validate in editor: Interact intent (Triggered/Started) executes current option and refreshes prompt spec; ensure router bind works on possessed pawns/controllers.
- Confirm prompt delegate consumers render correct text/verb and selection heuristic feels correct with blocked options.
- Consider adjusting selection persistence to follow OptionTag when options reorder.

ROLLBACK
- Revert touched driver files + docs listed above (or git revert the associated commit if created).
