#include "SOTS_MissionDirectorDebugLibrary.h"
#include "SOTS_MissionDirectorSubsystem.h"
#include "SOTS_MissionDirectorTypes.h"
#include "SOTS_MissionDirectorModule.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "SOTS_TagAccessHelpers.h"

void USOTS_MissionDirectorDebugLibrary::RunSOTS_MissionDirector_DebugRun(
    const UObject* WorldContextObject,
    float ExtraScoreDelta)
{
    if (!WorldContextObject)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("[Mission Debug] WorldContextObject is null."));
        return;
    }

    USOTS_MissionDirectorSubsystem* MD = USOTS_MissionDirectorSubsystem::Get(WorldContextObject);
    if (!MD)
    {
        UE_LOG(LogSOTSMissionDirector, Warning, TEXT("[Mission Debug] Mission Director subsystem not found."));
        return;
    }

    // Start a debug mission if none active.
    if (!MD->IsMissionActive())
    {
        const FGameplayTag DebugDifficulty; // empty for now
        MD->StartMissionRun(FName(TEXT("SOTS_Debug_Mission")), DebugDifficulty);
    }

    FGameplayTagContainer EmptyTags;

    // Register a primary objective completion.
    MD->RegisterObjectiveCompleted(
        /*bIsPrimaryObjective*/ true,
        /*ScoreDelta*/ 250.0f,
        NSLOCTEXT("SOTSMissionDebug", "PrimaryObjectiveTitle", "Debug Primary Objective"),
        NSLOCTEXT("SOTSMissionDebug", "PrimaryObjectiveDesc", "Mission Director integration test objective."),
        EmptyTags);

    // Optional extra score event for tuning.
    if (FMath::Abs(ExtraScoreDelta) > KINDA_SMALL_NUMBER)
    {
        MD->RegisterScoreEvent(
            ESOTSMissionEventCategory::Misc,
            ExtraScoreDelta,
            NSLOCTEXT("SOTSMissionDebug", "ExtraScoreTitle", "Debug Extra Score"),
            NSLOCTEXT("SOTSMissionDebug", "ExtraScoreDesc", "Additional debug score delta."),
            EmptyTags);
    }

    // End mission and print summary.
    FSOTS_MissionRunSummary Summary;
    MD->EndMissionRun(/*bSuccess*/ true, Summary);

    UE_LOG(LogSOTSMissionDirector, Log,
        TEXT("[Mission Debug] MissionId=%s Score=%.2f Rank=%s Duration=%.2fs Prim=%d Opt=%d Events=%d"),
        *Summary.MissionId.ToString(),
        Summary.FinalScore,
        *Summary.FinalRank.ToString(),
        Summary.DurationSeconds,
        Summary.PrimaryObjectivesCompleted,
        Summary.OptionalObjectivesCompleted,
        Summary.EventLog.Num());

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Green,
            FString::Printf(TEXT("[Mission Debug] %s | Score=%.2f Rank=%s Events=%d"),
                *Summary.MissionId.ToString(),
                Summary.FinalScore,
                *Summary.FinalRank.ToString(),
                Summary.EventLog.Num()));
    }
#endif
}

void USOTS_MissionDirectorDebugLibrary::SOTS_MarkMissionStarted(
    const UObject* WorldContextObject,
    FGameplayTag MissionStartTag)
{
    if (!WorldContextObject || !MissionStartTag.IsValid())
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(WorldContextObject))
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0))
        {
            TagSubsystem->AddTagToActor(PlayerPawn, MissionStartTag);
        }
    }
}

void USOTS_MissionDirectorDebugLibrary::SOTS_MarkMissionCompleted(
    const UObject* WorldContextObject,
    FGameplayTag MissionCompleteTag)
{
    if (!WorldContextObject || !MissionCompleteTag.IsValid())
    {
        return;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(WorldContextObject))
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0))
        {
            TagSubsystem->AddTagToActor(PlayerPawn, MissionCompleteTag);
        }
    }
}

bool USOTS_MissionDirectorDebugLibrary::SOTS_IsMissionCompleted(
    const UObject* WorldContextObject,
    FGameplayTag MissionCompleteTag)
{
    if (!WorldContextObject || !MissionCompleteTag.IsValid())
    {
        return false;
    }

    if (USOTS_GameplayTagManagerSubsystem* TagSubsystem = SOTS_GetTagSubsystem(WorldContextObject))
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0))
        {
            return TagSubsystem->ActorHasTag(PlayerPawn, MissionCompleteTag);
        }
    }

    return false;
}

bool USOTS_MissionDirectorDebugLibrary::ValidateMissionDefinition(const USOTS_MissionDefinition* MissionDef, FText& OutError)
{
    OutError = FText();

    if (!MissionDef)
    {
        OutError = NSLOCTEXT("SOTSMissionDirector", "Validation_NullMission", "Mission definition is null.");
        return false;
    }

    TSet<FName> RouteIds;
    TSet<FName> ObjectiveIds;

    // Collect routes and check uniqueness/non-null.
    for (const TObjectPtr<USOTS_RouteDefinition>& Route : MissionDef->Routes)
    {
        if (!Route)
        {
            OutError = NSLOCTEXT("SOTSMissionDirector", "Validation_NullRoute", "Mission contains a null route asset.");
            return false;
        }

        const FName RouteName = Route->RouteId.Id;
        if (RouteName.IsNone())
        {
            OutError = NSLOCTEXT("SOTSMissionDirector", "Validation_EmptyRouteId", "RouteId is empty.");
            return false;
        }

        if (RouteIds.Contains(RouteName))
        {
            OutError = FText::Format(NSLOCTEXT("SOTSMissionDirector", "Validation_DuplicateRoute", "Duplicate RouteId: {0}"), FText::FromName(RouteName));
            return false;
        }

        RouteIds.Add(RouteName);
    }

    // Collect global objectives.
    auto CollectObjective = [&](const TObjectPtr<USOTS_ObjectiveDefinition>& Obj, bool bRouteScoped) -> bool
    {
        if (!Obj)
        {
            OutError = bRouteScoped
                ? NSLOCTEXT("SOTSMissionDirector", "Validation_NullRouteObjective", "Route contains a null objective asset.")
                : NSLOCTEXT("SOTSMissionDirector", "Validation_NullGlobalObjective", "Mission contains a null global objective asset.");
            return false;
        }

        const FName ObjName = Obj->ObjectiveId.Id;
        if (ObjName.IsNone())
        {
            OutError = NSLOCTEXT("SOTSMissionDirector", "Validation_EmptyObjectiveId", "ObjectiveId is empty.");
            return false;
        }

        if (ObjectiveIds.Contains(ObjName))
        {
            OutError = FText::Format(NSLOCTEXT("SOTSMissionDirector", "Validation_DuplicateObjective", "Duplicate ObjectiveId: {0}"), FText::FromName(ObjName));
            return false;
        }

        ObjectiveIds.Add(ObjName);
        return true;
    };

    for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : MissionDef->GlobalObjectives)
    {
        if (!CollectObjective(Obj, /*bRouteScoped*/ false))
        {
            return false;
        }
    }

    for (const TObjectPtr<USOTS_RouteDefinition>& Route : MissionDef->Routes)
    {
        if (!Route)
        {
            continue; // already handled null above; defensive.
        }

        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : Route->Objectives)
        {
            if (!CollectObjective(Obj, /*bRouteScoped*/ true))
            {
                return false;
            }
        }
    }

    // Validate references: objective RequiresCompleted and AllowedRoutes.
    for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : MissionDef->GlobalObjectives)
    {
        if (!Obj)
        {
            continue;
        }

        for (const FSOTS_ObjectiveId& Prereq : Obj->RequiresCompleted)
        {
            if (!Prereq.Id.IsNone() && !ObjectiveIds.Contains(Prereq.Id))
            {
                OutError = FText::Format(NSLOCTEXT("SOTSMissionDirector", "Validation_BadPrereq", "Objective {0} requires missing objective {1}"), FText::FromName(Obj->ObjectiveId.Id), FText::FromName(Prereq.Id));
                return false;
            }
        }

        for (const FSOTS_RouteId& AllowedRoute : Obj->AllowedRoutes)
        {
            if (!AllowedRoute.Id.IsNone() && !RouteIds.Contains(AllowedRoute.Id))
            {
                OutError = FText::Format(NSLOCTEXT("SOTSMissionDirector", "Validation_BadAllowedRoute", "Objective {0} references missing route {1}"), FText::FromName(Obj->ObjectiveId.Id), FText::FromName(AllowedRoute.Id));
                return false;
            }
        }
    }

    for (const TObjectPtr<USOTS_RouteDefinition>& Route : MissionDef->Routes)
    {
        if (!Route)
        {
            continue;
        }

        for (const TObjectPtr<USOTS_ObjectiveDefinition>& Obj : Route->Objectives)
        {
            if (!Obj)
            {
                continue;
            }

            for (const FSOTS_ObjectiveId& Prereq : Obj->RequiresCompleted)
            {
                if (!Prereq.Id.IsNone() && !ObjectiveIds.Contains(Prereq.Id))
                {
                    OutError = FText::Format(NSLOCTEXT("SOTSMissionDirector", "Validation_BadPrereqRoute", "Route objective {0} requires missing objective {1}"), FText::FromName(Obj->ObjectiveId.Id), FText::FromName(Prereq.Id));
                    return false;
                }
            }

            for (const FSOTS_RouteId& AllowedRoute : Obj->AllowedRoutes)
            {
                if (!AllowedRoute.Id.IsNone() && !RouteIds.Contains(AllowedRoute.Id))
                {
                    OutError = FText::Format(NSLOCTEXT("SOTSMissionDirector", "Validation_BadAllowedRouteRoute", "Route objective {0} references missing route {1}"), FText::FromName(Obj->ObjectiveId.Id), FText::FromName(AllowedRoute.Id));
                    return false;
                }
            }
        }
    }

    return true;
}
