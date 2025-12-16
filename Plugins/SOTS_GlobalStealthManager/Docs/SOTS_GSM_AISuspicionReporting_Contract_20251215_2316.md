# SOTS GSM AI Suspicion Reporting Contract (2025-12-15)

## Data payload
- `FSOTS_AISuspicionReport`: `SubjectAI` (TWeak AActor), `Suspicion01`, `ReasonTag` (may be empty for legacy callers), `bHasLocation`, `Location`, `InstigatorActor` (TWeak AActor), `TimestampSeconds` (world time seconds).

## APIs
- `ReportAISuspicion(AActor* SubjectAI, float Suspicion01)`: legacy entrypoint; now forwards into Ex with empty reason/no location/instigator.
- `ReportAISuspicionEx(AActor* SubjectAI, float Suspicion01, FGameplayTag ReasonTag, const FVector& Location, bool bHasLocation, AActor* InstigatorActor)`: context-rich reporting; stores last report per AI, emits delegate, runs existing ingest/throttle path.
- `GetLastAISuspicionReport(AActor* SubjectAI, FSOTS_AISuspicionReport& OutReport) const`: returns last accepted report for the AI; false if none/stale.

## Events
- `FSOTS_OnAISuspicionReported OnAISuspicionReported`: broadcast whenever a suspicion report is accepted via the ingest path (legacy and Ex callers).

Notes: Old behavior preserved; existing min-delta/ingest validation still gate acceptance/broadcast. Legacy callers compile unchanged.
