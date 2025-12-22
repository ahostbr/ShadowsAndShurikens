#include "SOTS_SteamMissionResultBridgeSubsystem.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "SOTS_MissionDirectorSubsystem.h"
#include "SOTS_SteamAchievementsSubsystem.h"
#include "SOTS_SteamLeaderboardsSubsystem.h"

void USOTS_SteamMissionResultBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        MissionDirector = USOTS_MissionDirectorSubsystem::Get(GameInstance);

        if (MissionDirector.IsValid())
        {
            MissionDirector->OnMissionEnded.AddDynamic(
                this,
                &USOTS_SteamMissionResultBridgeSubsystem::HandleMissionEnded);
            bMissionEndedBound = true;
        }
    }

    LogVerbose(TEXT("Mission result bridge initialized."));
}

void USOTS_SteamMissionResultBridgeSubsystem::Deinitialize()
{
    if (MissionDirector.IsValid() && bMissionEndedBound)
    {
        MissionDirector->OnMissionEnded.RemoveDynamic(
            this,
            &USOTS_SteamMissionResultBridgeSubsystem::HandleMissionEnded);
    }

    MissionDirector.Reset();
    bMissionEndedBound = false;
    LastSubmissionTimeSeconds = 0.0;
    Super::Deinitialize();
}

void USOTS_SteamMissionResultBridgeSubsystem::HandleMissionEnded(const FSOTS_MissionRunSummary& Summary)
{
    const double NowSeconds = FPlatformTime::Seconds();
    if ((NowSeconds - LastSubmissionTimeSeconds) < static_cast<double>(SubmissionRateLimitSeconds))
    {
        LogVerbose(TEXT("HandleMissionEnded - rate limited; skipping duplicate work."));
        return;
    }

    LastSubmissionTimeSeconds = NowSeconds;

    const FSOTS_SteamMissionResult Result = BuildSteamMissionResult(Summary);
    SubmitToLeaderboards(Result);
    DispatchAchievements(Result);
}

FSOTS_SteamMissionResult USOTS_SteamMissionResultBridgeSubsystem::BuildSteamMissionResult(const FSOTS_MissionRunSummary& Summary) const
{
    FSOTS_SteamMissionResult Result;
    Result.MissionTag = BuildMissionTag(Summary.MissionId);
    Result.DifficultyTag = Summary.DifficultyTag;
    Result.Score = static_cast<int32>(FMath::Max(0.0f, Summary.FinalScore));
    Result.MissionTimeSeconds = Summary.DurationSeconds;

    FGameplayTagContainer AggregatedTags;
    AppendMissionEventTags(AggregatedTags, Summary);

    const FGameplayTag OutcomeTag = FGameplayTag::RequestGameplayTag(
        FName(Summary.bSuccess ? TEXT("MissionOutcome.Completed") : TEXT("MissionOutcome.Failed")),
        false);
    if (OutcomeTag.IsValid())
    {
        AggregatedTags.AddTag(OutcomeTag);
    }

    bool bNoKills = false;
    bool bNoAlerts = false;
    bool bPerfectStealth = false;
    ExtractModifierFlags(AggregatedTags, bNoKills, bNoAlerts, bPerfectStealth);

    Result.AdditionalTags = AggregatedTags;
    Result.bNoKills = bNoKills;
    Result.bNoAlerts = bNoAlerts;
    Result.bPerfectStealth = bPerfectStealth;

    return Result;
}

FGameplayTag USOTS_SteamMissionResultBridgeSubsystem::BuildMissionTag(const FName& MissionId) const
{
    if (MissionId.IsNone())
    {
        return FGameplayTag();
    }

    const FString TagString = FString::Printf(TEXT("SAS.Mission.%s"), *MissionId.ToString());
    return FGameplayTag::RequestGameplayTag(FName(*TagString), false);
}

void USOTS_SteamMissionResultBridgeSubsystem::AppendMissionEventTags(
    FGameplayTagContainer& OutTags,
    const FSOTS_MissionRunSummary& Summary) const
{
    for (const FSOTS_MissionEventLogEntry& Entry : Summary.EventLog)
    {
        OutTags.AppendTags(Entry.ContextTags);
    }
}

void USOTS_SteamMissionResultBridgeSubsystem::ExtractModifierFlags(
    const FGameplayTagContainer& Tags,
    bool& bOutNoKills,
    bool& bOutNoAlerts,
    bool& bOutPerfectStealth) const
{
    const struct
    {
        const TCHAR* Path;
        bool* Flag;
    } TagChecks[] = {
        {TEXT("SAS.Mission.NoKills"), &bOutNoKills},
        {TEXT("SAS.Mission.NoAlerts"), &bOutNoAlerts},
        {TEXT("SAS.Mission.PerfectStealth"), &bOutPerfectStealth}
    };

    for (const auto& Entry : TagChecks)
    {
        const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(Entry.Path), false);
        if (Tag.IsValid() && Tags.HasTag(Tag))
        {
            *Entry.Flag = true;
        }
    }
}

FString USOTS_SteamMissionResultBridgeSubsystem::GetCurrentPlayerName() const
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(World, 0))
        {
            if (APlayerState* PlayerState = Controller->PlayerState)
            {
                return PlayerState->GetPlayerName();
            }
        }
    }

    return FString();
}

void USOTS_SteamMissionResultBridgeSubsystem::SubmitToLeaderboards(const FSOTS_SteamMissionResult& Result)
{
    if (!GetGameInstance())
    {
        return;
    }

    if (USOTS_SteamLeaderboardsSubsystem* Leaderboards = GetGameInstance()->GetSubsystem<USOTS_SteamLeaderboardsSubsystem>())
    {
        Leaderboards->RequestLeaderboardData();

        const FString PlayerName = GetCurrentPlayerName();
        const bool bSubmitted = Leaderboards->SubmitMissionResultToLeaderboards(Result, PlayerName, true);

        if (bSubmitted)
        {
            LogVerbose(TEXT("SubmitToLeaderboards - mission result recorded locally."));
        }
        else
        {
            LogVerbose(TEXT("SubmitToLeaderboards - mission result was skipped (score disabled or leaderboards unavailable)."));
        }
    }
}

void USOTS_SteamMissionResultBridgeSubsystem::DispatchAchievements(const FSOTS_SteamMissionResult& Result)
{
    if (!GetGameInstance())
    {
        return;
    }

    if (USOTS_SteamAchievementsSubsystem* Achievements =
            GetGameInstance()->GetSubsystem<USOTS_SteamAchievementsSubsystem>())
    {
        Achievements->RequestCurrentStats();
        Achievements->HandleMissionResult(Result);
    }
}
