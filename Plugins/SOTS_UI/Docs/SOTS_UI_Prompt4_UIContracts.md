# SOTS UI Prompt 4 — UI Contracts & Modal Flow

Intent handling:
- Intent tags use gameplay tags (lightweight helpers in `SOTS_UIIntentTags.h`). No hardcoded registry list; tags are requested with `ErrorIfNotFound=false`.
- Built-in system actions handled by the router: `SAS.UI.Action.QuitGame`, `SAS.UI.Action.ReturnToMainMenu` (TODO hook game flow), `SAS.UI.Action.OpenSettings`, `SAS.UI.Action.OpenProfiles`, `SAS.UI.Action.CloseTopModal`.

Payload structs (instanced via `FInstancedStruct`):
- `F_SOTS_UIConfirmDialogPayload`
- `F_SOTS_UIPlayVideoPayload`
- `F_SOTS_UIOpenMenuPayload`
- `F_SOTS_UINotificationPayload`
- `F_SOTS_UIWaypointPayload`
- `F_SOTS_UIInventoryRequestPayload`

Modal result flow:
- Requester -> `Router.ShowConfirmDialog` (payload becomes instanced) -> Modal widget -> `Router.SubmitModalResult` -> `OnModalResult` broadcast -> Router pops the top modal (visual only; input priority unchanged).

Missing tags (set up in TagManager/DA):
- Confirm dialog widget ids: `UI.Modals.DialogPrompt` (primary), `SAS.UI.CGF.Modal.DialogPrompt` (fallback).
- System actions listed above (`SAS.UI.Action.*`).
