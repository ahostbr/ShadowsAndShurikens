# SOTS_FX Plugin — SPINE6 Shipping Sweep (2025-12-16 14:39)

## Guard checklist (dev-only, off by default)
- Library registration logs gated by build + `bLogLibraryRegistration` [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L148-L207).
- Library order debug gated by build + `bDebugLogCueResolution` [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L286-L338).
- Pool stats logging gated by build + `bDebugDumpPoolStats` [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L501-L542).
- FX failure logging gated by build + `bLogFXRequestFailures` [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L874-L896).
- Cue resolution debug gated by build + `bDebugLogCueResolution` [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1000-L1019).
- Pool lifecycle events gated by build + `bLogPoolActions`; release still executes silently in Shipping/Test [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1800-L1827).

## Editor leak status
- Scan found no WITH_EDITOR/Editor-only symbols in SOTS_FX_Plugin (ad_hoc_regex_search run).
- Build.cs pulls only runtime-safe modules [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/SOTS_FX_Plugin.Build.cs](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/SOTS_FX_Plugin.Build.cs#L6-L29).

## TODO / dead-end cleanup
- ad_hoc_regex_search found no TODO/FIXME/placeholder/.bak items inside SOTS_FX_Plugin.

## Contract verification
- Execute path centralized: `ExecuteCue` is sole execution entry; `RequestFXCue*` → `ProcessFXRequest` → `TryResolveCue` → `ExecuteCue` [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L620-L870).
- Deterministic resolution via `TryResolveCue` and cached library map [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L939-L1021).
- All public entrypoints return `FSOTS_FXRequestReport` or legacy struct with explicit status [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L820-L922).
- Failure paths emit reports and optional guarded logs; no void/do-nothing public APIs remain.

## SPINE 1–6 recap
1. SPINE_1 — Request/report contract: Blueprint-callable `RequestFXCue*` return `FSOTS_FXRequestReport` carrying status + debug [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L820-L922).
2. SPINE_2 — Deterministic library resolution via ordered cache with debug log guard [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L286-L338) and resolver [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L939-L1021).
3. SPINE_3 — Execution pipeline: context build, policy check, offsets, params, unified `ExecuteCue` [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L970-L1325).
4. SPINE_4 — Pooling/lifecycle: tag-based acquire/release, overflow policy, stats dump [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L400-L690) and pooled release events [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1700-L1827).
5. SPINE_5 — Policy/profile: global toggles enforced in `EvaluateFXPolicy` with camera shake opt-out [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L897-L966) and profile setters in header defaults [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Public/SOTS_FXManagerSubsystem.h#L47-L120).
6. SPINE_6 — Shipping sweep: all debug logs behind `#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)` + config booleans; scans confirm no editor-only leakage or TODOs.

## Evidence pointers
- Library registration guard example [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L148-L207).
- Pool stats guard example [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L501-L542).
- Failure logging guard example [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L874-L896).
- Pool lifecycle guard example [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/Private/SOTS_FXManagerSubsystem.cpp#L1800-L1827).
- Editor-safety via Build.cs deps [Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/SOTS_FX_Plugin.Build.cs](Plugins/SOTS_FX_Plugin/Source/SOTS_FX_Plugin/SOTS_FX_Plugin.Build.cs#L6-L29).

## Cleanup
- Per instruction: delete Binaries/ and Intermediate for SOTS_FX_Plugin after sweep (see shell log).
