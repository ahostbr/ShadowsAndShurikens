# SOTS_AIPerception SPINE4 — Senses & Targeting (2025-12-14 02:05)

## Sense ➜ ReasonTag mapping
- Mapping is centralized in `MapSenseToReasonTag` (Sight/Hearing/Shadow/Damage ➜ Config reason tags, fallback Generic) [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1572-L1614](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1572-L1614).

## Target resolution policy (player-only)
- Primary target is resolved once via `UGameplayStatics::GetPlayerPawn(World,0)` and cached [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1513-L1536](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1513-L1536).
- Refresh prunes all other targets and keeps only the primary [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L583-L610](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L583-L610).

## Last stimulus cache (authoritative)
- Cache struct fields: Strength01, SenseTag, WorldLocation, SourceActor, TimestampSeconds, bSuccessfullySensed, Age helper [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L81-L106](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Public/SOTS_AIPerceptionTypes.h#L81-L106).
- Updates only for the primary target; prefers successful stimuli and higher strength when timestamps match [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1544-L1568](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1544-L1568).
- Hearing stimulus caches strength/location and marks GSM intent [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L546-L577](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L546-L577).
- Sight/shadow stimuli cache actor/location when LOS or shadow hit occurs [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L792-L806](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L792-L806) and [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L856-L872](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L856-L872).

## Dominant sense evaluation feeding suspicion/telemetry
- `EvaluateStimuliForTarget` picks the strongest sense (Sight/Hearing/Shadow) and reports dominance [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1596-L1610](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1596-L1610).
- Perception tick evaluates stimuli, feeds `UpdateSuspicionModel`, and falls back to mapped sense tags for GSM reporting [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L140-L182](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L140-L182).
- Suspicion model consumes the precomputed dominant sense and maps it via the centralized table before broadcasting/telemetry [Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1705-L1778](Plugins/SOTS_AIPerception/Source/SOTS_AIPerception/Private/SOTS_AIPerceptionComponent.cpp#L1705-L1778).
