# SOTS AIPerception Blackboard Output Policy (Selectors-first)

- Canonical writes use Blackboard selectors when `SelectedKeyName` is set. Legacy keys are only written when the selector for that value is unset/invalid.
- Selector fields consumed from `FSOTS_AIPerceptionBlackboardConfig`: `TargetActorKey`, `SuspicionKey`, `StateKey`, `LastKnownLocationKey`, `LastKnownLocationValidKey`, `HasLOSKey`.
- Legacy fallback keys (written only when the matching selector is missing): Target actor -> `TargetActor`; Has LOS -> `HasLOSToTarget`; Last known location -> `LastKnownTargetLocation`; Suspicion/Awareness -> `Awareness` (per-target awareness); Perception state -> `PerceptionState`. There is no legacy key for last-known-location validity; when invalid and no selector is provided, the location vector is cleared.
- Unified write path: `USOTS_AIPerceptionComponent::WriteBlackboardSnapshot` is the sole place that writes BB values. `UpdateBlackboardAndTags` routes all component updates through this helper.
- Warning behavior: if any selector is missing, the component logs once (`LogSOTS_AIPerception`, Warning) per instance noting legacy fallback and sets `bWarnedAboutLegacyBBFallback` to avoid spam. To remove the warning, set the selectors in the config asset.
