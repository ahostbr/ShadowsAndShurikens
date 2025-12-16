# Buddy Worklog — SOTS_Input SPINE_6 (2025-12-16)

## Goal
Final polish for SOTS_Input: stable public include, integration contract, perf pass (no per-frame tick by default, fewer allocations), optional async registry loads, docs/lock checkpoint.

## Changes made
- Added stable consumer include `SOTS_InputAPI.h` and marked public-facing classes as STABLE API.
- Optimized router dispatch: precomputed flat dispatch list + layer boundaries (no per-event allocations), timer-based auto-refresh (opt-in, default off), tick now only for debug logging.
- Added buffer capacity guard (`MaxBufferedEventsPerChannel`) with reserve and drop-oldest logging in dev builds.
- Added async-optional layer registry loading with StreamableManager, caching, and sync fallback; router handles async-miss gracefully.
- Documented integration contract and LOCK checkpoint; updated overview to SPINE_6 and referenced stable include + async/timer behaviors.

## Files changed
- Source/SOTS_Input/Public/SOTS_InputAPI.h (new)
- Source/SOTS_Input/Public/SOTS_InputRouterComponent.h
- Source/SOTS_Input/Private/SOTS_InputRouterComponent.cpp
- Source/SOTS_Input/Public/SOTS_InputBufferComponent.h
- Source/SOTS_Input/Private/SOTS_InputBufferComponent.cpp
- Source/SOTS_Input/Public/SOTS_InputLayerRegistrySubsystem.h
- Source/SOTS_Input/Private/SOTS_InputLayerRegistrySubsystem.cpp
- Source/SOTS_Input/Public/SOTS_InputLayerRegistrySettings.h
- Source/SOTS_Input/Private/SOTS_InputLayerRegistrySettings.cpp
- Source/SOTS_Input/Public/SOTS_InputBlueprintLibrary.h
- Source/SOTS_Input/Public/SOTS_InputLayerDataAsset.h
- Source/SOTS_Input/Public/SOTS_InputHandler.h
- Docs/SOTS_Input_Overview.md
- Docs/SOTS_Input_IntegrationContract.md (new)
- Docs/SOTS_Input_LOCK.md (new)

## Perf/tick notes
- Router tick is disabled by default; only enabled for debug logging.
- Auto-refresh uses a timer (0.5s default) when `bEnableAutoRefresh` is true; no per-frame polling.
- Dispatch uses cached arrays; no per-event TArray allocations from layer evaluation.
- Buffering limits per-channel (default 16) with drop-oldest in dev logging.

## Async registry notes
- Developer setting `bUseAsyncLoads` controls async StreamableManager requests; sync fallback remains when disabled.
- Router logs verbosely (non-Shipping/Test) when awaiting async layer loads.

## Cleanup
- Deleted Plugins/SOTS_Input/Binaries/ and Plugins/SOTS_Input/Intermediate/ (no build run).
