# Buddy Worklog - BEHAVIOR_SWEEP_19_SaveTriggers_TravelChain (20251222_092409)

Goal
- Finish save trigger wiring with KEM save-block respect and keep the travel chain ProfileShared → UI → MissionDirector clean (UI entrypoints only).

What changed
- ProfileShared: added autosave timer scaffolding and “save with checks” helpers. Saves now refuse while KEM is active (IsSaveBlocked), expose UI-friendly reason, track active profile ID (set on load/save), and provide RequestSaveCurrentProfile/RequestCheckpointSave. Autosave timer (default enabled, 300s) runs only when an active profile is set.
- ProfileShared: added KEM save-block query for UI (`IsSaveBlockedForUI`) and made SaveProfile short-circuit if KEM is active.
- UI Router: new save entrypoints `RequestSaveGame` and `RequestCheckpointSave` that call ProfileShared, respect KEM blocking, and surface success/fail via notifications. Added dependency on ProfileShared and Profile types.

Files changed
- Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Public/SOTS_ProfileSubsystem.h
- Plugins/SOTS_ProfileShared/Source/SOTS_ProfileShared/Private/SOTS_ProfileSubsystem.cpp
- Plugins/SOTS_UI/Source/SOTS_UI/SOTS_UI.Build.cs
- Plugins/SOTS_UI/Source/SOTS_UI/Public/SOTS_UIRouterSubsystem.h
- Plugins/SOTS_UI/Source/SOTS_UI/Private/SOTS_UIRouterSubsystem.cpp

Notes / Decisions
- Travel chain stays UI-owned: UI exposes save requests; ProfileShared performs the work; no MissionDirector compile-time dependency added. KEM save-block is enforced at ProfileShared entrypoints and surfaced to UI notifications.
- Autosave uses a timer (no tick); runs only when ActiveProfileId is set.

Verification
- Not run (Token Guard; no builds/tests).

Cleanup
- Deleted Binaries/Intermediate for SOTS_ProfileShared and SOTS_UI.
