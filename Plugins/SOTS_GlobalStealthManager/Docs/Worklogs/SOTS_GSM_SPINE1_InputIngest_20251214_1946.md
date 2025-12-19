# SOTS_GSM_SPINE1_InputIngest_20251214_1946

Scope: GSM input ingestion determinism and diagnostics; no tier/modifier/tag changes.

Updated public entrypoints (all now route through IngestSample)
- `ReportStealthSample` -> `IngestSample` before scoring (`Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Private/SOTS_GlobalStealthManagerSubsystem.cpp::ReportStealthSample`).
- `ReportEnemyDetectionEvent` -> `IngestSample` before detection state update (`...Subsystem.cpp::ReportEnemyDetectionEvent`).
- `ReportAISuspicion` -> `IngestSample` before guard map aggregation (`...Subsystem.cpp::ReportAISuspicion`).

New normalized ingest surface
- `FSOTS_StealthInputSample`, `ESOTS_StealthInputType`, `FSOTS_StealthIngestReport`, `ESOTS_StealthIngestResult` (`Plugins/SOTS_GlobalStealthManager/Source/SOTS_GlobalStealthManager/Public/SOTS_GlobalStealthTypes.h`).
- Config hook: `FSOTS_StealthIngestTuning` + `FSOTS_StealthScoringConfig::IngestTuning` (same file) with default-off throttle and logging flags.

Central ingest + validation
- `USOTS_GlobalStealthManagerSubsystem::IngestSample` (clamp 0..1, reject NaN/Inf, timestamp repair, optional stale drop, optional per-type throttle) and `LogIngestDecision` (dev-only, throttled) in `...Subsystem.cpp`.
- Helpers: `GetWorldTimeSecondsSafe`, `GetMinSecondsBetweenType` keep timing deterministic even without world; per-type last accepted/log maps ensure throttle decisions are deterministic across sources (`...Subsystem.h/.cpp`).

Throttle/time-window (defaults preserve prior behavior)
- `FSOTS_StealthIngestTuning`: `bEnableIngestThrottle=false`, `bDropStaleSamples=false`, per-type `MinSecondsBetween*` (all 0), `MaxSampleAgeSeconds=1.0f` (only used when stale drop is enabled), dev log throttling (`DebugLogThrottleSeconds=1.0f`).
- Throttle logic uses `LastAcceptedSampleTime` by `ESOTS_StealthInputType`; drop reasons set to `Dropped_TooSoon`/`Dropped_OutOfWindow` with debug reason for diagnostics (`...Subsystem.cpp::IngestSample`).

Dev-only diagnostics
- Logging gated by `FSOTS_StealthIngestTuning::bDebugLogStealthIngest` / `bDebugLogStealthDrops`; compiled out in shipping/test; throttled per type via `LastLoggedIngestTime`/`LastLoggedDropTime` (`...Subsystem.cpp::LogIngestDecision`).

Notes
- Defaults keep previous immediate behavior (no throttles, no stale drop, no logging) while enabling deterministic validation and traceable drop reasons when debug flags are toggled.
