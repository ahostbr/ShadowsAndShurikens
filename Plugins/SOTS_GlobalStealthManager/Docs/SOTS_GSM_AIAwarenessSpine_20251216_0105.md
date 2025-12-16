# SOTS GSM AI Awareness Spine (2025-12-16)

Scope: Awareness state/record handling for `ReportAISuspicionEx` in GSM. No behavior beyond the contract below.

## States and thresholds
- Enum: Calm, Investigating, Searching, Alerted, Hostile (see `ESOTS_AIAwarenessState`).
- Thresholds (normalized 0-1) live in `FSOTS_GSM_AwarenessThresholds` inside the GSM scoring config:
  - InvestigatingMin01=0.25, SearchingMin01=0.50, AlertedMin01=0.80, HostileMin01=0.95.
- Resolution: clamp suspicion, compare from Hostile down to Calm.

## Records and accessors
- `FSOTS_GSM_AIRecord`: SubjectAI, Suspicion01, AwarenessState, AwarenessTierTag, ReasonTag, FocusTag, bHasLocation, Location, InstigatorActor, TimestampSeconds.
- Storage: `AIRecords` keyed by subject (WeakObject). Invalid keys are pruned on ingest and reset.
- Accessors: `GetAIRecord`, `GetAwarenessStateForAI`, legacy `GetLastAISuspicionReport` falls back to AIRecord data when needed.

## Events
- `OnAISuspicionReported` (existing): fired after ingest acceptance.
- `OnAIRecordUpdated` (new): fired after record update every accepted report.
- `OnAIAwarenessStateChanged` (new): fires only when awareness state changes for the subject (old/new + record payload).

## Tag publishing (on the subject AI)
- Tier tags: `SAS.AI.Alert.{Calm,Investigating,Searching,Alerted,Hostile}` (remove all, apply current tier).
- Reason tags: `SAS.AI.Reason.{Sight,Hearing,Shadow,Damage,Generic}` (remove all, apply last reason when valid).
- Focus tags: `SAS.AI.Focus.{Player,Unknown}` (remove both, apply current focus tag).

## Aggregation into GSM score
- `ReportAISuspicionEx` updates/creates an AIRecord, resolves awareness state, applies tags, then recomputes the max suspicion across valid records.
- `SetAISuspicion(MaxSuspicion)` remains the single path into `CurrentState.AISuspicion01`/global recompute.

## Reset behavior
- `ResetStealthState` clears `AIRecords`, `GuardSuspicion`, and `LastAISuspicionReports` in addition to existing GSM state.

Notes: Reason/focus tags are registered under `SAS.AI.*` and added/removed via `USOTS_TagLibrary`. Defaults keep legacy callers working; callers that only pass Suspicion01 still get state/tags via the generic reason fallback.
