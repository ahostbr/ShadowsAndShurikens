# Buddy Worklog — BEHAVIOR_SWEEP_14 AIPerception (UDS Breadcrumb Assist) (2025-12-21 21:34)

- Exposed UDS breadcrumb pull helper (`SOTS_GetRecentUDSBreadcrumbs`) that wraps the UDSBridge API and returns ordered location/time/world data for AIBT consumption without binding to UDS internals.
- Added lightweight breadcrumb data struct to AIPerception types and hooked module dependency on SOTS_UDSBridge for the optional helper.
- No changes to core stimulus→GSM/FX paths; behavior remains timer/event driven with existing GSM reporting and FX one-shots.
- No builds or runtime execution per instructions; kept logging gated to Verbose when breadcrumb pull fails.
- Cleanup: deleted Plugins/SOTS_AIPerception/Binaries and Plugins/SOTS_AIPerception/Intermediate (paths absent OK).
