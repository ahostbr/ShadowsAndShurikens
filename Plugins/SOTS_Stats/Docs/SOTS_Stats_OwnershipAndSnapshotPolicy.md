# SOTS Stats Ownership & Snapshot Policy

## Ownership
- The single authoritative storage for stats is `USOTS_StatsComponent` (BlueprintSpawnable actor component). Every pawn that participates in the suite should get its own component instance so that stat maps stay isolated per actor.
- `USOTS_StatsComponent` keeps three maps (`StatValues`, `StatMinValues`, `StatMaxValues`) and exposes helpers like `SetStatValue`, `AddToStat`, `SetStatBounds`, and `SetAllStats`. `InternalSetStat` is the only setter and it respects bounds before broadcasting `OnStatChanged`.
- Global consumers do not reach into the component directly; instead they call the helper library (`USOTS_StatsLibrary`) which looks up the component on the actor (`FindComponentByClass`) and routes `GetActorStatValue`, `AddToActorStat`, or `SetActorStatValue` calls through the component.

## Snapshot contract
- Stats data copies into/out of `FSOTS_CharacterStateData` from `Plugins/SOTS_ProfileShared`. The struct is part of `FSOTS_ProfileSnapshot`, so the profile snapshot that lands in `SOTS_ProfileShared` is the authoritative source of truth for persisted stat values.
- `USOTS_StatsComponent::WriteToCharacterState` / `BuildCharacterStateData` simply copy the `StatValues` map into the snapshot, and `ReadFromCharacterState` / `ApplyCharacterStateData` copy it back verbatim. No recomputation occurs on load beyond the existing clamp logic in `InternalSetStat`.
- When the profile loader feeds `FSOTS_ProfileSnapshot.PlayerCharacter`, it should route that slice straight through `USOTS_StatsLibrary::ApplySnapshotToActor` so stat values match the saved payload and the component becomes the runtime authoritative map.

## Integration seams
- **ProfileShared → Stats:** `USOTS_StatsLibrary::ApplySnapshotToActor` is the obvious hook for profile restores. Callers already have the snapshot, so pass `Snapshot.PlayerCharacter` to that helper instead of replaying individual deltas.
- **MissionDirector → SkillTree → Stats → UI:** MissionDirector persistence helpers (see `SOTS_MissionDirector_Persistence_UIIntents_20251215_1345.md` and its `TryWriteMilestoneToStatsAndProfile`) collect mission reward buckets, let the skill tree translate them into unlocked nodes/values, and finally drive stats by calling the library helpers. The UI only observes stats by reading from the component or listening to `OnStatChanged`, so keep this chain intact – MD should never bypass skill tree or StatsLibrary when mutating values.
- **UI → Stats:** Treat stats as read-only data; query `GetStatValue` / `GetAllStats` or use `USOTS_StatsLibrary::GetActorStatValue` and subscribe to `OnStatChanged` for change notifications. Do not invent another writable cache layer.

## Eventing & notifications
- `USOTS_StatsComponent::OnStatChanged` is dispatched whenever a stat actually changes (see `InternalSetStat`). UI observers can subscribe to this delegate and pull the latest value instead of polling every frame.
- `SetStatBounds` exists so designers can clamp diurnal ranges without the UI needing to double-check bounds; the component re-applies bounds immediately when the clamp maps change.
- To avoid event spam, rely on the `OnStatChanged` delegate and trust that `InternalSetStat` suppresses duplicate broadcasts when the incoming value equals the stored value. Consumers can still batch UI updates if a burst of stats fires, but the component already guarantees only real deltas bubble up.

## How to use (snippet)
```cpp
if (APawn* Pawn = /* owning pawn */)
{
    if (USOTS_StatsComponent* Stats = Pawn->FindComponentByClass<USOTS_StatsComponent>())
    {
        Stats->OnStatChanged.AddDynamic(this, &ThisClass::HandleStatChanged);
        const float Current = Stats->GetStatValue(MyStatTag);
    }
}
```

## Non-goals (code stage)
- Do not add data assets/configs or new save/load plumbing in this pass; document the policy first.
- Do not introduce additional writable caching layers between MissionDirector, SkillTree, Stats, and the UI; keep the call flow MD → SkillTree → StatsLibrary → UI.