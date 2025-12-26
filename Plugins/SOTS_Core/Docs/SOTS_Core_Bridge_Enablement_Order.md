# SOTS_Core Bridge Enablement Order (BRIDGE4)

This document is the **golden switch-flip sequence** for enabling SOTS_Core bridges (BRIDGE1–BRIDGE3) safely.
Everything is designed to be **OFF by default** and **rollbackable via config** (no Blueprint edits required for the bridge layer).

## Bridge participants (current)

### SOTS_Core (dispatch + diagnostics)
- Settings class: `USOTS_CoreSettings`
- Config section: `[/Script/SOTS_Core.SOTS_CoreSettings]`
- Key switches:
  - `bEnableLifecycleListenerDispatch` (default `false`) — allows SOTS_Core to call registered listeners.
  - `bEnableLifecycleDispatchLogs` (default `false`) — verbose Core dispatch logging.

Diagnostics console commands (non-shipping/test):
- `SOTS.Core.DumpSettings`
- `SOTS.Core.Health`
- `SOTS.Core.DumpLifecycle`
- `SOTS.Core.DumpListeners`
- `SOTS.Core.DumpSaveParticipants`

### SOTS_Input (BRIDGE1)
- Settings class: `USOTS_InputCoreBridgeSettings`
- Config section: `[/Script/SOTS_Input.SOTS_InputCoreBridgeSettings]`
- Switches (default OFF):
  - `bEnableSOTSCoreLifecycleBridge`
  - `bEnableSOTSCoreBridgeVerboseLogs`

### SOTS_ProfileShared (BRIDGE2)
- Settings class: `USOTS_ProfileSharedCoreBridgeSettings`
- Config section: `[/Script/SOTS_ProfileShared.SOTS_ProfileSharedCoreBridgeSettings]`
- Switches (default OFF):
  - `bEnableSOTSCoreLifecycleBridge`
  - `bEnableSOTSCoreBridgeVerboseLogs`

### SOTS_UI (BRIDGE3)
- Settings class: `USOTS_UISettings`
- Config section: `[/Script/SOTS_UI.SOTS_UISettings]`
- Switches (default OFF):
  - `bEnableSOTSCoreHUDHostBridge`
  - `bEnableSOTSCoreBridgeVerboseLogs`

Notes:
- The bridge listeners may be **registered at module startup** even when their bridge switch is OFF.
  - Behavior is still inert because each listener callback **early-outs** unless the bridge switch is enabled.

## Phase A (safe): verify Core only
Goal: confirm SOTS_Core is present and inert, and diagnostics work.

1) Keep all bridge settings OFF (defaults).
2) In an editor PIE/world context, run:
   - `SOTS.Core.DumpSettings` (expects: `LifecycleDispatch=false`)
   - `SOTS.Core.Health` (expects: `SOTS_Core Health Report:`)
   - `SOTS.Core.DumpLifecycle` (expects: `SOTS_Core Snapshot:`)

What to look for:
- Logs showing settings + snapshot without warnings about missing subsystems.

## Phase B: enable Core lifecycle dispatch
Goal: allow SOTS_Core to dispatch lifecycle callbacks to registered listeners.

1) Set in config:
```
[/Script/SOTS_Core.SOTS_CoreSettings]
bEnableLifecycleListenerDispatch=true
; optional:
bEnableLifecycleDispatchLogs=false
```
2) Run:
- `SOTS.Core.DumpSettings` (confirm `LifecycleDispatch=true`)
- `SOTS.Core.DumpListeners` (see how many listeners are registered)

Optional:
- Turn on `bEnableLifecycleDispatchLogs=true` to observe Core dispatch activity.

## Phase C: enable bridges one at a time
Goal: turn on ONE bridged plugin at a time and verify expected state/log signals.

### Step C1 — Enable SOTS_Input BRIDGE1
1) Set:
```
[/Script/SOTS_Input.SOTS_InputCoreBridgeSettings]
bEnableSOTSCoreLifecycleBridge=true
; optional:
bEnableSOTSCoreBridgeVerboseLogs=false
```
2) What to look for:
- If verbose logs enabled: `SOTS_Input CoreBridge:` messages when PC BeginPlay / PawnPossessed occur.
- No input mode changes are expected (state-only wiring).

### Step C2 — Enable SOTS_ProfileShared BRIDGE2
1) Set:
```
[/Script/SOTS_ProfileShared.SOTS_ProfileSharedCoreBridgeSettings]
bEnableSOTSCoreLifecycleBridge=true
; optional:
bEnableSOTSCoreBridgeVerboseLogs=false
```
2) What to look for:
- If verbose logs enabled: `ProfileShared CoreBridge:` messages for WorldStartPlay / PostLogin / PawnPossessed.
- No IO/autosave/travel should be triggered by this bridge.

### Step C3 — Enable SOTS_UI BRIDGE3
1) Set:
```
[/Script/SOTS_UI.SOTS_UISettings]
bEnableSOTSCoreHUDHostBridge=true
; optional:
bEnableSOTSCoreBridgeVerboseLogs=false
```
2) What to look for:
- If verbose logs enabled: `Router: Registered HUD host via SOTS_Core bridge ...`
- No widget creation or router stack changes are expected from BRIDGE3 (host pointer is state-only).

## Rollback (always safe)
1) Toggle OFF the per-plugin bridge settings.
2) Optionally set `bEnableLifecycleListenerDispatch=false` in SOTS_Core.
3) No Blueprint changes are required for bridge rollback.

## Dependency notes (FYI)
- SOTS_Input, SOTS_ProfileShared, and SOTS_UI declare a C++ dependency on `SOTS_Core` via their `.Build.cs`.
