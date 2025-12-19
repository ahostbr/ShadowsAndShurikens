# Shipping/Test Safety — SOTS_BPGen_Bridge

- Module type: Editor-only (TCP NDJSON bridge for BPGen). Keep runtime modules free of this dependency.
- Apply/mutation operations should still respect `SOTS_ALLOW_APPLY=1` in upstream callers; bridge does not bypass gating.
- Default bind address/port remain editor-only; avoid exposing to packaged builds. No automatic start in runtime targets.
- Logging should stay at default verbosity; avoid spam in packaged builds.
