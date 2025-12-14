#include "SOTS_SteamLeaderboardsSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "OnlineStats.h"
#include "SOTS_SteamSettings.h"
#include "SOTS_SteamLeaderboardSaveGame.h"
#include "SOTS_SteamLog.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Math/UnrealMathUtility.h"
#include "GameplayTagContainer.h"

void USOTS_SteamLeaderboardsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bLeaderboardsLoaded = false;
    CurrentSaveGame = nullptr;
    CachedRegistry.Reset();

    LogVerbose(TEXT("USOTS_SteamLeaderboardsSubsystem initialized."));
}

void USOTS_SteamLeaderboardsSubsystem::Deinitialize()
{
    LogVerbose(TEXT("USOTS_SteamLeaderboardsSubsystem deinitializing."));

    CurrentSaveGame = nullptr;
    CachedRegistry.Reset();
    bLeaderboardsLoaded = false;

    Super::Deinitialize();
}

void USOTS_SteamLeaderboardsSubsystem::SetCurrentProfileId(const FString& InProfileId)
{
    CurrentProfileId = InProfileId;
    bLeaderboardsLoaded = false;
    CurrentSaveGame = nullptr;
    CachedRegistry.Reset();

    FString LogMsg = FString::Printf(TEXT("SetCurrentProfileId (Leaderboards): '%s'"), *CurrentProfileId);
    LogVerbose(LogMsg);
}

FString USOTS_SteamLeaderboardsSubsystem::GetSaveSlotName() const
{
    if (CurrentProfileId.IsEmpty())
    {
        return TEXT("SOTS_Steam_Leaderboards_Global");
    }

    return FString::Printf(TEXT("SOTS_Steam_Leaderboards_%s"), *CurrentProfileId);
}

const USOTS_SteamLeaderboardRegistry* USOTS_SteamLeaderboardsSubsystem::GetLeaderboardRegistry()
{
    if (CachedRegistry.IsValid())
    {
        return CachedRegistry.Get();
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    if (!Settings)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("GetLeaderboardRegistry - Settings are null."));
        return nullptr;
    }

    USOTS_SteamLeaderboardRegistry* Registry = nullptr;

    if (LeaderboardRegistryOverride.IsValid())
    {
        Registry = LeaderboardRegistryOverride.Get();
    }
    else if (!Settings->DefaultLeaderboardRegistry.IsNull())
    {
        Registry = Settings->DefaultLeaderboardRegistry.LoadSynchronous();
    }

    if (!Registry)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("GetLeaderboardRegistry - No registry assigned (override or settings)."));
        return nullptr;
    }

    CachedRegistry = Registry;
    return Registry;
}

USOTS_SteamLeaderboardSaveGame* USOTS_SteamLeaderboardsSubsystem::GetOrCreateSaveGame()
{
    if (CurrentSaveGame)
    {
        return CurrentSaveGame;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("GetOrCreateSaveGame (Leaderboards) - World is null."));
        return nullptr;
    }

    const FString SlotName = GetSaveSlotName();
    const int32 UserIndex = 0;

    if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
        CurrentSaveGame = Cast<USOTS_SteamLeaderboardSaveGame>(Loaded);

        if (!CurrentSaveGame)
        {
            UE_LOG(LogSOTS_Steam, Warning, TEXT("GetOrCreateSaveGame (Leaderboards) - Existing save is not USOTS_SteamLeaderboardSaveGame. Creating new."));
        }
    }

    if (!CurrentSaveGame)
    {
        CurrentSaveGame = Cast<USOTS_SteamLeaderboardSaveGame>(
            UGameplayStatics::CreateSaveGameObject(USOTS_SteamLeaderboardSaveGame::StaticClass()));
    }

    return CurrentSaveGame;
}

bool USOTS_SteamLeaderboardsSubsystem::RequestLeaderboardData()
{
    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    const bool bVerbose = Settings && Settings->bEnableVerboseLogging;
    if (!Settings || !Settings->bEnableLeaderboards)
    {
        if (bVerbose)
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("RequestLeaderboardData - Leaderboards disabled in settings; treating as no-op."));
        }
        bLeaderboardsLoaded = true;
        return true;
    }

    USOTS_SteamLeaderboardSaveGame* SaveGame = GetOrCreateSaveGame();
    if (!SaveGame)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("RequestLeaderboardData - Failed to get or create SaveGame."));
        return false;
    }

    bLeaderboardsLoaded = true;

    FString LogMsg = FString::Printf(
        TEXT("RequestLeaderboardData - Loaded data for slot '%s' (Profile: '%s')."),
        *GetSaveSlotName(),
        *CurrentProfileId);
    LogVerbose(LogMsg);

    return true;
}

bool USOTS_SteamLeaderboardsSubsystem::StoreLeaderboardData()
{
    if (!bLeaderboardsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("StoreLeaderboardData - Data not loaded. Call RequestLeaderboardData first."));
        return false;
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    const bool bVerbose = Settings && Settings->bEnableVerboseLogging;
    if (!Settings || !Settings->bEnableLeaderboards)
    {
        if (bVerbose)
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("StoreLeaderboardData - Leaderboards disabled in settings; treating as no-op."));
        }
        return true;
    }

    USOTS_SteamLeaderboardSaveGame* SaveGame = GetOrCreateSaveGame();
    if (!SaveGame)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("StoreLeaderboardData - Failed to get or create SaveGame."));
        return false;
    }

    const FString SlotName = GetSaveSlotName();
    const int32 UserIndex = 0;

    if (!UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex))
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("StoreLeaderboardData - SaveGameToSlot failed for '%s'."), *SlotName);
        return false;
    }

    FString LogMsg = FString::Printf(
        TEXT("StoreLeaderboardData - Saved data for slot '%s' (Profile: '%s')."),
        *SlotName,
        *CurrentProfileId);
    LogVerbose(LogMsg);

    return true;
}

void USOTS_SteamLeaderboardsSubsystem::DumpLeaderboardsToLog() const
{
    const USOTS_SteamLeaderboardSaveGame* SaveGame = CurrentSaveGame;
    if (!SaveGame)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("DumpLeaderboardsToLog - No saved leaderboard data available."));
        return;
    }

    if (SaveGame->LeaderboardStates.IsEmpty())
    {
        UE_LOG(LogSOTS_Steam, Log, TEXT("DumpLeaderboardsToLog - No cached leaderboard entries."));
        return;
    }

    for (const FSOTS_SteamLeaderboardBoardState& Entry : SaveGame->LeaderboardStates)
    {
        UE_LOG(
            LogSOTS_Steam,
            Log,
            TEXT("Leaderboard '%s': BestScore=%d, LastScore=%d, PlayerName='%s'"),
            *Entry.InternalId.ToString(),
            Entry.BestScore,
            Entry.LastScore,
            *Entry.PlayerName);
    }
}

FSOTS_SteamLeaderboardBoardState* USOTS_SteamLeaderboardsSubsystem::FindOrAddBoardState(FName InternalId)
{
    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("FindOrAddBoardState - InternalId is None."));
        return nullptr;
    }

    USOTS_SteamLeaderboardSaveGame* SaveGame = GetOrCreateSaveGame();
    if (!SaveGame)
    {
        return nullptr;
    }

    for (FSOTS_SteamLeaderboardBoardState& State : SaveGame->LeaderboardStates)
    {
        if (State.InternalId == InternalId)
        {
            return &State;
        }
    }

    FSOTS_SteamLeaderboardBoardState NewState;
    NewState.InternalId = InternalId;
    SaveGame->LeaderboardStates.Add(NewState);

    return &SaveGame->LeaderboardStates.Last();
}

bool USOTS_SteamLeaderboardsSubsystem::IsBetterScore(FName InternalId, int32 Candidate, int32 Existing) const
{
    const USOTS_SteamLeaderboardRegistry* Registry = const_cast<USOTS_SteamLeaderboardsSubsystem*>(this)->GetLeaderboardRegistry();
    if (!Registry)
    {
        return Candidate > Existing;
    }

    const FSOTS_SteamLeaderboardDefinition* Def = Registry->FindByInternalId(InternalId);
    if (!Def)
    {
        return Candidate > Existing;
    }

    if (Def->bHigherIsBetter)
    {
        return Candidate > Existing;
    }

    if (Existing == 0)
    {
        return Candidate > 0;
    }

    return Candidate > 0 && Candidate < Existing;
}

bool USOTS_SteamLeaderboardsSubsystem::SubmitScore(
    FName InternalId,
    int32 Score,
    const FString& PlayerName,
    bool bOnlyIfBetter)
{
    if (!bLeaderboardsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("SubmitScore - Data not loaded. Call RequestLeaderboardData first."));
        return false;
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    const bool bVerbose = Settings && Settings->bEnableVerboseLogging;
    if (!Settings || !Settings->bEnableLeaderboards)
    {
        if (bVerbose)
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("SubmitScore - Leaderboards disabled in settings; treating as no-op."));
        }
        return true;
    }

    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("SubmitScore - InternalId is None."));
        return false;
    }

    FSOTS_SteamLeaderboardBoardState* State = FindOrAddBoardState(InternalId);
    if (!State)
    {
        return false;
    }

    const int32 OldBest = State->BestScore;
    const bool bHadBest = (OldBest != 0);

    State->LastScore = Score;

    bool bIsNewBest = false;
    if (!bOnlyIfBetter || !bHadBest || IsBetterScore(InternalId, Score, OldBest))
    {
        State->BestScore = Score;
        bIsNewBest = true;
    }

    if (!PlayerName.IsEmpty())
    {
        State->PlayerName = PlayerName;
    }

    OnScoreSubmitted.Broadcast(InternalId, Score, bIsNewBest);
    OnLeaderboardUpdated.Broadcast(InternalId);

    TryPushScoreToOnline(InternalId, Score, nullptr, bOnlyIfBetter);

    return true;
}

bool USOTS_SteamLeaderboardsSubsystem::GetBestScore(FName InternalId, int32& OutScore) const
{
    OutScore = 0;

    if (!bLeaderboardsLoaded)
    {
        if (USOTS_SteamSettings::IsVerboseLoggingEnabled())
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("GetBestScore - Data not loaded."));
        }
        return false;
    }

    USOTS_SteamLeaderboardsSubsystem* MutableThis = const_cast<USOTS_SteamLeaderboardsSubsystem*>(this);
    USOTS_SteamLeaderboardSaveGame* SaveGame = MutableThis->GetOrCreateSaveGame();
    if (!SaveGame)
    {
        return false;
    }

    for (const FSOTS_SteamLeaderboardBoardState& State : SaveGame->LeaderboardStates)
    {
        if (State.InternalId == InternalId)
        {
            OutScore = State.BestScore;
            return (OutScore != 0);
        }
    }

    return false;
}

bool USOTS_SteamLeaderboardsSubsystem::GetTopEntries(
    FName InternalId,
    int32 MaxEntries,
    TArray<FSOTS_SteamLeaderboardRow>& OutEntries) const
{
    OutEntries.Reset();

    if (!bLeaderboardsLoaded)
    {
        if (USOTS_SteamSettings::IsVerboseLoggingEnabled())
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("GetTopEntries - Data not loaded."));
        }
        return false;
    }

    if (MaxEntries <= 0)
    {
        return false;
    }

    USOTS_SteamLeaderboardsSubsystem* MutableThis = const_cast<USOTS_SteamLeaderboardsSubsystem*>(this);
    USOTS_SteamLeaderboardSaveGame* SaveGame = MutableThis->GetOrCreateSaveGame();
    if (!SaveGame)
    {
        return false;
    }

    for (const FSOTS_SteamLeaderboardBoardState& State : SaveGame->LeaderboardStates)
    {
        if (State.InternalId == InternalId && State.BestScore != 0)
        {
            FSOTS_SteamLeaderboardRow Row;
            Row.InternalId = InternalId;
            Row.Rank = 1;
            Row.PlayerName = State.PlayerName.IsEmpty() ? TEXT("Player") : State.PlayerName;
            Row.Score = State.BestScore;
            OutEntries.Add(Row);
            break;
        }
    }

    return OutEntries.Num() > 0;
}

bool USOTS_SteamLeaderboardsSubsystem::GetAroundPlayer(
    FName InternalId,
    int32 Range,
    TArray<FSOTS_SteamLeaderboardRow>& OutEntries) const
{
    const int32 EffectiveRange = FMath::Max(1, Range);
    return GetTopEntries(InternalId, EffectiveRange, OutEntries);
}

FGameplayTagContainer USOTS_SteamLeaderboardsSubsystem::BuildMissionResultTags(const FSOTS_SteamMissionResult& Result) const
{
    FGameplayTagContainer Tags;

    if (Result.MissionTag.IsValid())
    {
        Tags.AddTag(Result.MissionTag);
    }

    if (Result.DifficultyTag.IsValid())
    {
        Tags.AddTag(Result.DifficultyTag);
    }

    if (Result.bNoKills)
    {
        Tags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.Mission.NoKills")), false));
    }

    if (Result.bNoAlerts)
    {
        Tags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.Mission.NoAlerts")), false));
    }

    if (Result.bPerfectStealth)
    {
        Tags.AddTag(FGameplayTag::RequestGameplayTag(FName(TEXT("SAS.Mission.PerfectStealth")), false));
    }

    Tags.AppendTags(Result.AdditionalTags);

    return Tags;
}

bool USOTS_SteamLeaderboardsSubsystem::ShouldUseOnlineLeaderboards() const
{
    if (!IsSteamAvailable())
    {
        return false;
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    if (!Settings)
    {
        return false;
    }

    if (!Settings->bEnableLeaderboards)
    {
        return false;
    }

    if (!Settings->bEnableSteamIntegration || !Settings->bUseSteamForLeaderboards)
    {
        return false;
    }

    return true;
}

IOnlineSubsystem* USOTS_SteamLeaderboardsSubsystem::GetOnlineSubsystemSteam() const
{
    return GetOnlineSubsystemSafe();
}

IOnlineIdentityPtr USOTS_SteamLeaderboardsSubsystem::GetOnlineIdentityInterface() const
{
    IOnlineSubsystem* Subsystem = GetOnlineSubsystemSteam();
    if (!Subsystem)
    {
        return nullptr;
    }

    return Subsystem->GetIdentityInterface();
}

IOnlineLeaderboardsPtr USOTS_SteamLeaderboardsSubsystem::GetOnlineLeaderboardsInterface() const
{
    IOnlineSubsystem* Subsystem = GetOnlineSubsystemSteam();
    if (!Subsystem)
    {
        return nullptr;
    }

    return Subsystem->GetLeaderboardsInterface();
}

TSharedPtr<const FUniqueNetId> USOTS_SteamLeaderboardsSubsystem::GetPrimaryUserId() const
{
    IOnlineIdentityPtr Identity = GetOnlineIdentityInterface();
    if (!Identity.IsValid())
    {
        return nullptr;
    }

    return Identity->GetUniquePlayerId(0);
}

bool USOTS_SteamLeaderboardsSubsystem::TryPushScoreToOnline(
    FName InternalId,
    int32 Score,
    const FSOTS_SteamLeaderboardDefinition* Def,
    bool bOnlyIfBetter) const
{
    if (!ShouldUseOnlineLeaderboards())
    {
        return false;
    }

    const FSOTS_SteamLeaderboardDefinition* ResolvedDef = Def;
    if (!ResolvedDef)
    {
        const USOTS_SteamLeaderboardRegistry* Registry = const_cast<USOTS_SteamLeaderboardsSubsystem*>(this)->GetLeaderboardRegistry();
        if (Registry)
        {
            ResolvedDef = Registry->FindByInternalId(InternalId);
        }
    }

    if (!ResolvedDef || !ResolvedDef->bMirrorToSteam)
    {
        return false;
    }

    if (Score <= 0)
    {
        return false;
    }

    IOnlineLeaderboardsPtr Leaderboards = GetOnlineLeaderboardsInterface();
    IOnlineIdentityPtr Identity = GetOnlineIdentityInterface();
    if (!Leaderboards.IsValid() || !Identity.IsValid())
    {
        return false;
    }

    const TSharedPtr<const FUniqueNetId> UserId = GetPrimaryUserId();
    if (!UserId.IsValid())
    {
        return false;
    }

    const FName ApiName = !ResolvedDef->SteamApiName.IsNone() ? ResolvedDef->SteamApiName : ResolvedDef->InternalId;
    if (ApiName.IsNone())
    {
        return false;
    }

    const FName RatedStatName = !ResolvedDef->RatedStatName.IsNone() ? ResolvedDef->RatedStatName : ApiName;
    const FString ApiNameStr = ApiName.ToString();
    const FString RatedStatStr = RatedStatName.ToString();

    FOnlineLeaderboardWrite WriteObject;
    WriteObject.LeaderboardNames.Empty();
    WriteObject.LeaderboardNames.Add(ApiNameStr);
    WriteObject.DisplayFormat = ELeaderboardFormat::Number;
    WriteObject.RatedStat = RatedStatStr;
    WriteObject.SortMethod = ELeaderboardSort::Descending;
    WriteObject.UpdateMethod = bOnlyIfBetter
        ? ELeaderboardUpdateMethod::KeepBest
        : ELeaderboardUpdateMethod::Force;
    WriteObject.SetIntStat(RatedStatStr, Score);

    static const FName SessionName = NAME_GameSession;
    const bool bStarted = Leaderboards->WriteLeaderboards(SessionName, *UserId, WriteObject);
    if (!bStarted)
    {
        return false;
    }

    Leaderboards->FlushLeaderboards(SessionName);
    return true;
}

bool USOTS_SteamLeaderboardsSubsystem::SubmitMissionResultToLeaderboards(
    const FSOTS_SteamMissionResult& Result,
    const FString& PlayerName,
    bool bOnlyIfBetter)
{
    if (!bLeaderboardsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("SubmitMissionResultToLeaderboards - Data not loaded. Call RequestLeaderboardData first."));
        return false;
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    const bool bVerbose = Settings && Settings->bEnableVerboseLogging;
    if (!Settings || !Settings->bEnableLeaderboards)
    {
        if (bVerbose)
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("SubmitMissionResultToLeaderboards - Leaderboards disabled in settings; treating as no-op."));
        }
        return true;
    }

    const USOTS_SteamLeaderboardRegistry* Registry = GetLeaderboardRegistry();
    if (!Registry)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("SubmitMissionResultToLeaderboards - No leaderboard registry available."));
        return false;
    }

    if (Result.Score <= 0)
    {
        if (USOTS_SteamSettings::IsVerboseLoggingEnabled())
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("SubmitMissionResultToLeaderboards - Result.Score <= 0, skipping submission."));
        }
        return false;
    }

    const FGameplayTagContainer MissionTags = BuildMissionResultTags(Result);

    bool bAnySubmitted = false;

    for (const FSOTS_SteamLeaderboardDefinition& Def : Registry->Leaderboards)
    {
        if (Def.InternalId.IsNone())
        {
            continue;
        }

        if (Def.Tags.Num() == 0)
        {
            continue;
        }

        if (MissionTags.HasAll(Def.Tags))
        {
            if (SubmitScore(Def.InternalId, Result.Score, PlayerName, bOnlyIfBetter))
            {
                bAnySubmitted = true;
            }
        }
    }

    return bAnySubmitted;
}

void USOTS_SteamLeaderboardsSubsystem::BuildRowsFromCurrentRead(TArray<FSOTS_SteamLeaderboardRow>& OutEntries) const
{
    OutEntries.Reset();

    if (!CurrentReadObject.IsValid())
    {
        return;
    }

    const FName InternalId = CurrentReadInternalId;
    const FName RatedStatName = CurrentReadRatedStatName;

    for (const FOnlineStatsRow& Row : CurrentReadObject->Rows)
    {
        FSOTS_SteamLeaderboardRow Entry;
        Entry.InternalId = InternalId;
        Entry.Rank = static_cast<int32>(Row.Rank);
        Entry.PlayerName = Row.NickName;

        int32 Score = 0;
        if (RatedStatName != NAME_None)
        {
            const FVariantData* ScoreData = Row.Columns.Find(RatedStatName.ToString());
            if (ScoreData)
            {
                ScoreData->GetValue(Score);
            }
        }

        Entry.Score = Score;
        OutEntries.Add(Entry);
    }
}

void USOTS_SteamLeaderboardsSubsystem::HandleSteamLeaderboardReadComplete(bool bWasSuccessful)
{
    IOnlineLeaderboardsPtr Leaderboards = GetOnlineLeaderboardsInterface();
    if (Leaderboards.IsValid() && LeaderboardReadCompleteDelegateHandle.IsValid())
    {
        Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
        LeaderboardReadCompleteDelegateHandle.Reset();
    }

    TArray<FSOTS_SteamLeaderboardRow> Entries;
    if (bWasSuccessful)
    {
        BuildRowsFromCurrentRead(Entries);
    }

    if (CurrentQueryKind == ESOTS_SteamLeaderboardQueryKind::Top)
    {
        OnSteamTopEntriesReceived.Broadcast(CurrentReadInternalId, Entries, bWasSuccessful);
    }
    else if (CurrentQueryKind == ESOTS_SteamLeaderboardQueryKind::AroundPlayer)
    {
        OnSteamAroundPlayerEntriesReceived.Broadcast(CurrentReadInternalId, Entries, bWasSuccessful);
    }

    CurrentQueryKind = ESOTS_SteamLeaderboardQueryKind::None;
    CurrentReadInternalId = NAME_None;
    CurrentReadRatedStatName = NAME_None;
    CurrentReadObject.Reset();
}

void USOTS_SteamLeaderboardsSubsystem::QuerySteamTopEntries(FName InternalId, int32 NumEntries)
{
    NumEntries = FMath::Max(NumEntries, 1);

    if (!bLeaderboardsLoaded)
    {
        RequestLeaderboardData();
    }

    TArray<FSOTS_SteamLeaderboardRow> LocalEntries;
    GetTopEntries(InternalId, NumEntries, LocalEntries);

    if (!ShouldUseOnlineLeaderboards())
    {
        OnSteamTopEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const USOTS_SteamLeaderboardRegistry* Registry = GetLeaderboardRegistry();
    if (!Registry)
    {
        OnSteamTopEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const FSOTS_SteamLeaderboardDefinition* Def = Registry->FindByInternalId(InternalId);
    if (!Def || !Def->bMirrorToSteam)
    {
        OnSteamTopEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    IOnlineLeaderboardsPtr Leaderboards = GetOnlineLeaderboardsInterface();
    IOnlineIdentityPtr Identity = GetOnlineIdentityInterface();
    TSharedPtr<const FUniqueNetId> UserId = GetPrimaryUserId();

    if (!Leaderboards.IsValid() || !Identity.IsValid() || !UserId.IsValid())
    {
        OnSteamTopEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const FName ApiName = !Def->SteamApiName.IsNone() ? Def->SteamApiName : Def->InternalId;
    if (ApiName.IsNone())
    {
        OnSteamTopEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const FName RatedStatName = !Def->RatedStatName.IsNone() ? Def->RatedStatName : ApiName;
    const FString ApiNameStr = ApiName.ToString();
    const FString RatedStatStr = RatedStatName.ToString();

    CurrentReadObject = MakeShared<FOnlineLeaderboardRead, ESPMode::ThreadSafe>();
    CurrentReadObject->LeaderboardName = ApiNameStr;
    CurrentReadObject->SortedColumn = RatedStatStr;

    FColumnMetaData ColumnMeta(RatedStatStr, EOnlineKeyValuePairDataType::Int32);
    CurrentReadObject->ColumnMetadata.Add(ColumnMeta);

    CurrentReadInternalId = InternalId;
    CurrentReadRatedStatName = RatedStatName;
    CurrentQueryKind = ESOTS_SteamLeaderboardQueryKind::Top;

    if (LeaderboardReadCompleteDelegateHandle.IsValid())
    {
        Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
        LeaderboardReadCompleteDelegateHandle.Reset();
    }

    FOnLeaderboardReadCompleteDelegate Delegate = FOnLeaderboardReadCompleteDelegate::CreateUObject(
        this,
        &USOTS_SteamLeaderboardsSubsystem::HandleSteamLeaderboardReadComplete);

    LeaderboardReadCompleteDelegateHandle = Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(Delegate);

    FOnlineLeaderboardReadRef ReadRef = CurrentReadObject.ToSharedRef();
    const bool bStarted = Leaderboards->ReadLeaderboardsAroundRank(1, static_cast<uint32>(NumEntries), ReadRef);
    if (!bStarted)
    {
        if (LeaderboardReadCompleteDelegateHandle.IsValid())
        {
            Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
            LeaderboardReadCompleteDelegateHandle.Reset();
        }

        CurrentQueryKind = ESOTS_SteamLeaderboardQueryKind::None;
        CurrentReadInternalId = NAME_None;
        CurrentReadRatedStatName = NAME_None;
        CurrentReadObject.Reset();

        OnSteamTopEntriesReceived.Broadcast(InternalId, LocalEntries, false);
    }
}

void USOTS_SteamLeaderboardsSubsystem::QuerySteamAroundPlayer(FName InternalId, int32 Range)
{
    Range = FMath::Max(Range, 1);

    if (!bLeaderboardsLoaded)
    {
        RequestLeaderboardData();
    }

    TArray<FSOTS_SteamLeaderboardRow> LocalEntries;
    GetAroundPlayer(InternalId, Range, LocalEntries);

    if (!ShouldUseOnlineLeaderboards())
    {
        OnSteamAroundPlayerEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const USOTS_SteamLeaderboardRegistry* Registry = GetLeaderboardRegistry();
    if (!Registry)
    {
        OnSteamAroundPlayerEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const FSOTS_SteamLeaderboardDefinition* Def = Registry->FindByInternalId(InternalId);
    if (!Def || !Def->bMirrorToSteam)
    {
        OnSteamAroundPlayerEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    IOnlineLeaderboardsPtr Leaderboards = GetOnlineLeaderboardsInterface();
    IOnlineIdentityPtr Identity = GetOnlineIdentityInterface();
    const TSharedPtr<const FUniqueNetId> UserId = GetPrimaryUserId();

    if (!Leaderboards.IsValid() || !Identity.IsValid() || !UserId.IsValid())
    {
        OnSteamAroundPlayerEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const FName ApiName = !Def->SteamApiName.IsNone() ? Def->SteamApiName : Def->InternalId;
    if (ApiName.IsNone())
    {
        OnSteamAroundPlayerEntriesReceived.Broadcast(InternalId, LocalEntries, false);
        return;
    }

    const FName RatedStatName = !Def->RatedStatName.IsNone() ? Def->RatedStatName : ApiName;
    const FString ApiNameStr = ApiName.ToString();
    const FString RatedStatStr = RatedStatName.ToString();

    CurrentReadObject = MakeShared<FOnlineLeaderboardRead, ESPMode::ThreadSafe>();
    CurrentReadObject->LeaderboardName = ApiNameStr;
    CurrentReadObject->SortedColumn = RatedStatStr;

    FColumnMetaData ColumnMeta(RatedStatStr, EOnlineKeyValuePairDataType::Int32);
    CurrentReadObject->ColumnMetadata.Add(ColumnMeta);

    CurrentReadInternalId = InternalId;
    CurrentReadRatedStatName = RatedStatName;
    CurrentQueryKind = ESOTS_SteamLeaderboardQueryKind::AroundPlayer;

    if (LeaderboardReadCompleteDelegateHandle.IsValid())
    {
        Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
        LeaderboardReadCompleteDelegateHandle.Reset();
    }

    FOnLeaderboardReadCompleteDelegate Delegate = FOnLeaderboardReadCompleteDelegate::CreateUObject(
        this,
        &USOTS_SteamLeaderboardsSubsystem::HandleSteamLeaderboardReadComplete);

    LeaderboardReadCompleteDelegateHandle = Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(Delegate);

    FOnlineLeaderboardReadRef ReadRef = CurrentReadObject.ToSharedRef();
    FUniqueNetIdRef UserIdRef = UserId.ToSharedRef();

    const bool bStarted = Leaderboards->ReadLeaderboardsAroundUser(
        UserIdRef,
        static_cast<uint32>(Range),
        ReadRef);

    if (!bStarted)
    {
        if (LeaderboardReadCompleteDelegateHandle.IsValid())
        {
            Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
            LeaderboardReadCompleteDelegateHandle.Reset();
        }

        CurrentQueryKind = ESOTS_SteamLeaderboardQueryKind::None;
        CurrentReadInternalId = NAME_None;
        CurrentReadRatedStatName = NAME_None;
        CurrentReadObject.Reset();

        OnSteamAroundPlayerEntriesReceived.Broadcast(InternalId, LocalEntries, false);
    }
}
