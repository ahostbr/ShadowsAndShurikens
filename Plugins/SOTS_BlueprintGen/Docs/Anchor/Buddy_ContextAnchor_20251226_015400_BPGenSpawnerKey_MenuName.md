Send2SOTS
[CONTEXT_ANCHOR]
ID: 20251226_0154 | Plugin: SOTS_BlueprintGen | Pass/Topic: BPGenSpawnerKeyMenuName | Owner: Buddy
Scope: Allow BPGen ApplyGraphSpec to resolve spawner keys by UI menu name; update BeginPlay notification example.

DONE
- Updated spawner resolution to accept `SpawnerKey == Spawner->PrimeDefaultUiSpec().MenuName` (menu-name fallback).
- Updated example apply payload to use `apply_graph_spec_to_target` for EventGraph.
- Updated example GraphSpec to use spawner keys `Event BeginPlay` and `Get SOTS_UIRouterSubsystem` and fixed Delay exec output pin name to `then`.

VERIFIED
- None.

UNVERIFIED / ASSUMPTIONS
- Requires rebuild + editor run: menu-name spawner resolution is not verified in a live editor session.
- Assumes menu names are stable enough and unique enough for the specific nodes used (`Event BeginPlay`, `Get SOTS_UIRouterSubsystem`).
- Plugin artifact cleanup (Binaries/Intermediate) was attempted but blocked by a locked `UnrealEditor-SOTS_BlueprintGen.dll` (access denied).

FILES TOUCHED
- Plugins/SOTS_BlueprintGen/Source/SOTS_BlueprintGen/Private/SOTS_BPGenSpawnerRegistry.cpp
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_ApplyPayload.json
- Plugins/SOTS_BlueprintGen/Examples/BPGEN_BeginPlayNotification_GraphSpec.json

NEXT (Ryan)
- Rebuild and load plugin; re-run BPGen bridge apply for the example payload.
- Confirm spawned nodes resolve and link correctly (BeginPlay → Delay → ShowNotification) and notification appears at runtime.

ROLLBACK
- Revert the touched files (or `git revert` the change) to restore prior spawner resolution and example payload/spec.
