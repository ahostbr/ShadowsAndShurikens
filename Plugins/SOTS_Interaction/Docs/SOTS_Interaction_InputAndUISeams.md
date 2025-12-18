# SOTS Interaction — Input & UI Seams

## Purpose
- Wire the interaction driver to SOTS_Input intent tags without creating UI.
- Publish prompt data for SOTS_UI (or other consumers) via a multicast delegate.
- Keep tag gating boundary-safe (TagManager handles shared tags in subsystem).

## Input Intent Handling
- Interact intent tag: `Input.Intent.Gameplay.Interact` (emitted by `USOTS_InputHandler_Interact`).
- Driver entrypoints:
  - `HandleInputIntentTag(IntentTag)` → consumes the interact tag and calls `TryInteract` using the current selection (or fallback first available).
  - `BindToInputRouter()` → optional Blueprint-callable helper that binds `OnInputIntent` from the router to the driver.
- Router binding: `BeginPlay` attempts to get/ensure a `USOTS_InputRouterComponent` on the owner/controller and auto-binds `HandleRouterIntentEvent` (fires on Triggered/Started).

## Prompt Surface
- `FSOTS_InteractionPromptSpec` published via `OnInteractionPromptChanged` whenever focus/options/selection change.
- Fields: `FocusedActor`, `bHasFocus`, `bShowPrompt`, `PromptText`, `SuggestedVerbTag`, `Options`, `SelectedOptionIndex`.
- Selection rules:
  - Persist previous index if still valid; otherwise pick the highest-priority unblocked option.
  - If all options are blocked, choose the highest-priority option for display (still marked blocked via `BlockedReasonTag`).
- Prompt text/verb come from the selected option when focused; otherwise empty/interaction type tag.

## UI/Router Expectations
- Driver never creates widgets or pushes UI. Downstream systems should subscribe to `OnInteractionPromptChanged` and route intents through SOTS_UI.
- Tag gating remains in the subsystem (TagManager union view); driver only forwards prompt data and consumes intents.

## Debug/Notes
- If `OnInteractionPromptChanged` stops firing, ensure the router is present and bound (call `BindToInputRouter` in BP for late spawns).
- No build performed; runtime behavior unverified in-editor.
