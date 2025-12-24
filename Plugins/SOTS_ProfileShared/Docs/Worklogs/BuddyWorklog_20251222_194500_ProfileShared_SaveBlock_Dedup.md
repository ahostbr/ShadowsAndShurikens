## Summary
- Removed redundant duplicate definitions of `IsSaveBlockedByKEM` / `IsSaveBlockedForUI`, keeping the reflection-based soft dependency so ProfileShared no longer reaches for KEM types directly.
- Confirmed save-block check now resolves KEM via soft class path only; forward declaration removed to avoid hard coupling.
- Cleared `Plugins/SOTS_ProfileShared/Binaries` and `Intermediate` after the edit (no builds run).

## Notes
- No behavior change beyond deduplication; save-block gate still respects KEM activity through soft lookup.
