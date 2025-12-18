[CONTEXT_ANCHOR] 2025-12-18 — Code Review Stage Progress Checkpoint

## Stage policy (LOCKED)
- CODE-ONLY review: verify code surfaces exist + are correct; runtime validation later by Ryan.
- Shipping/shipping-hygiene + DataAssets/config wiring are deferred.
- GameplayTags audit is add-only and NOT deferred (Ryan has already performed the sweep).
- Behavior ownership decisions are locked suite-wide.

## Suite-wide ownership (LOCKED highlights)
- Travel chain: ProfileShared > UI > MissionDirector
- Save triggers: time autosave, UI-initiated, checkpoint
- UI owns UI input policy (InputMode/focus decisions)
- FX: systems call FXManager one-shots
- TagManager required only at cross-plugin boundary actor-state tags (Player/Dragon/Guards/Pickups); internal/local tags may bypass

## Completed worklogs / confirmed-done items (code-only)
### SOTS_TagManager
- MICRO pass completed (2025-12-18 00:05):
  - GetActorTags exposed as BlueprintCallable union view (Owned via IGameplayTagAssetInterface + loose + scoped/handle tags)
  - TagLibrary WorldContext overload added
  - TagBoundary.md updated
  - Cleanup: Binaries/Intermediate removed
  - No build/run

### SOTS_Input
- SPINE_2 completed (2025-12-18 03:05):
  - Minimal montage buffer windows (Execution + Vanish/QTE only)
  - Queue size 1, latest-wins, consume/clear
  - AnimNotifyState_SOTS_InputBufferWindow opens/closes windows (default Execution)
  - Auto-clear backstop via AnimInstance delegates on montage end/cancel/abort
  - Intent handler buffers into Execution/Vanish only when windows open
  - Added tag(s) in SOTS_InputTags.ini + docs
  - Cleanup done; no build/run
- SPINE_3 completed (2025-12-18 03:15):
  - Device-change seam (ReportInputDeviceFromKey alias, IsGamepadActive, BP helper)
  - Dragon control layer push/pop helpers for Input.Layer.Dragon.Control
  - Docs updated; cleanup done; no build/run

### SOTS_UI (InvSP adapter surface)
- MICRO “InvSPAdapter surface” completed (worklog 2025-12-17 20:56):
  - USOTS_InvSPAdapter C++ surface expanded (toggle, shortcut visibility, pickup notifications)
  - Safe defaults/no-ops (Toggle may call OpenInventory)
  - No build/run; cleanup noted

## Current truth about InvSP calls
- The expanded adapter surface exists, but the C++ “backing” owner + stable RequestInvSP_* entrypoints are still needed so the suite can call through SOTS_UI cleanly.
- Actual InvSP Blueprint node calls remain intentionally deferred until Ryan implements them in BP (not a Buddy task during this code-only stage).

## Next action (READY — queued prompt)
### SOTS_UI_BRIDGE_InvSP_CPPBacking (C++ only)
- Add C++ owner for InvSP adapter instance (router/subsystem)
- Add EnsureInvSPAdapter + RequestInvSP_* BlueprintCallable functions that forward into adapter BlueprintNativeEvents
- No BP assets, no InvSP deps, no build
- Worklog + cleanup required

## SOTS_Interaction status
- Buddy has reviewed existing implementation (subsystem selection/LOS/scoring + driver forwarding + interactable defaults + trace helpers).
- SPINE_1/2/3 prompts authored, but NO SPINE implementation changes performed yet.
- Recommendation: do SPINE_1 hardening first (driver API + delegates, hybrid provider determinism, tunables surface, “no UI creation” verification, TagManager union gating where boundary), then move on; SPINE_2/3 can be deferred until wiring pickups/execution/drag is imminent.

## File placement suggestion (per workflow)
- Save this anchor as:
  - Plugins/SOTS_UI/Docs/Anchors/CONTEXT_ANCHOR_20251218.md
  - (or the appropriate plugin anchor folder for the next plugin you lock)
