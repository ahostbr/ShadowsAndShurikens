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
