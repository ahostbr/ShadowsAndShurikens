#include "SOTS_SteamAchievementsSubsystem.h"

#include "SOTS_SteamSettings.h"
#include "SOTS_SteamAchievementSaveGame.h"
#include "SOTS_SteamLog.h"
#include "OnlineSubsystem.h"
#include "OnlineStats.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "GameplayTagContainer.h"

void USOTS_SteamAchievementsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bStatsLoaded = false;
    CurrentSaveGame = nullptr;
    CachedRegistry.Reset();

    LogVerbose(TEXT("USOTS_SteamAchievementsSubsystem initialized."));
}

void USOTS_SteamAchievementsSubsystem::Deinitialize()
{
    LogVerbose(TEXT("USOTS_SteamAchievementsSubsystem deinitializing."));

    CurrentSaveGame = nullptr;
    CachedRegistry.Reset();
    bStatsLoaded = false;

    Super::Deinitialize();
}

void USOTS_SteamAchievementsSubsystem::SetCurrentProfileId(const FString& InProfileId)
{
    CurrentProfileId = InProfileId;
    bStatsLoaded = false;
    CurrentSaveGame = nullptr;
    CachedRegistry.Reset();

    FString LogMsg = FString::Printf(TEXT("SetCurrentProfileId: '%s'"), *CurrentProfileId);
    LogVerbose(LogMsg);
}

FString USOTS_SteamAchievementsSubsystem::GetSaveSlotName() const
{
    if (CurrentProfileId.IsEmpty())
    {
        return TEXT("SOTS_Steam_Achievements_Global");
    }

    return FString::Printf(TEXT("SOTS_Steam_Achievements_%s"), *CurrentProfileId);
}

const USOTS_SteamAchievementRegistry* USOTS_SteamAchievementsSubsystem::GetAchievementRegistry()
{
    if (CachedRegistry.IsValid())
    {
        return CachedRegistry.Get();
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    if (!Settings)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("USOTS_SteamAchievementsSubsystem::GetAchievementRegistry - Settings are null."));
        return nullptr;
    }

    USOTS_SteamAchievementRegistry* Registry = nullptr;

    if (AchievementRegistryOverride.IsValid())
    {
        Registry = AchievementRegistryOverride.Get();
    }
    else if (!Settings->DefaultAchievementRegistry.IsNull())
    {
        Registry = Settings->DefaultAchievementRegistry.LoadSynchronous();
    }

    if (!Registry)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("USOTS_SteamAchievementsSubsystem::GetAchievementRegistry - No registry assigned (override or settings)."));
        return nullptr;
    }

    CachedRegistry = Registry;
    return Registry;
}

USOTS_SteamAchievementSaveGame* USOTS_SteamAchievementsSubsystem::GetOrCreateSaveGame()
{
    if (CurrentSaveGame)
    {
        return CurrentSaveGame;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("USOTS_SteamAchievementsSubsystem::GetOrCreateSaveGame - World is null."));
        return nullptr;
    }

    const FString SlotName = GetSaveSlotName();
    const int32 UserIndex = 0;

    if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
        CurrentSaveGame = Cast<USOTS_SteamAchievementSaveGame>(Loaded);

        if (!CurrentSaveGame)
        {
            UE_LOG(LogSOTS_Steam, Warning, TEXT("USOTS_SteamAchievementsSubsystem::GetOrCreateSaveGame - Existing save is not USOTS_SteamAchievementSaveGame. Creating new."));
        }
    }

    if (!CurrentSaveGame)
    {
        CurrentSaveGame = Cast<USOTS_SteamAchievementSaveGame>(UGameplayStatics::CreateSaveGameObject(USOTS_SteamAchievementSaveGame::StaticClass()));
    }

    return CurrentSaveGame;
}

bool USOTS_SteamAchievementsSubsystem::RequestCurrentStats()
{
    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    const bool bVerbose = Settings && Settings->bEnableVerboseLogging;
    if (!Settings || !Settings->bEnableAchievements)
    {
        if (bVerbose)
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("RequestCurrentStats - Achievements disabled in settings; treating as no-op."));
        }
        bStatsLoaded = true;
        return true;
    }

    USOTS_SteamAchievementSaveGame* SaveGame = GetOrCreateSaveGame();
    if (!SaveGame)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("RequestCurrentStats - Failed to get or create SaveGame."));
        return false;
    }

    bStatsLoaded = true;

    FString LogMsg = FString::Printf(
        TEXT("RequestCurrentStats - Loaded stats for slot '%s' (Profile: '%s')."),
        *GetSaveSlotName(),
        *CurrentProfileId);
    LogVerbose(LogMsg);

    return true;
}

bool USOTS_SteamAchievementsSubsystem::StoreStats()
{
    if (!bStatsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("StoreStats - Stats have not been loaded. Call RequestCurrentStats first."));
        return false;
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    const bool bVerbose = Settings && Settings->bEnableVerboseLogging;
    if (!Settings || !Settings->bEnableAchievements)
    {
        if (bVerbose)
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("StoreStats - Achievements disabled in settings; treating as no-op."));
        }
        return true;
    }

    USOTS_SteamAchievementSaveGame* SaveGame = GetOrCreateSaveGame();
    if (!SaveGame)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("StoreStats - Failed to get or create SaveGame."));
        return false;
    }

    const FString SlotName = GetSaveSlotName();
    const int32 UserIndex = 0;

    if (!UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex))
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("StoreStats - SaveGameToSlot failed for '%s'."), *SlotName);
        return false;
    }

    FString LogMsg = FString::Printf(
        TEXT("StoreStats - Saved stats for slot '%s' (Profile: '%s')."),
        *SlotName,
        *CurrentProfileId);
    LogVerbose(LogMsg);

    return true;
}

FSOTS_SteamAchievementState* USOTS_SteamAchievementsSubsystem::FindOrAddState(FName InternalId)
{
    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("FindOrAddState - InternalId is None."));
        return nullptr;
    }

    USOTS_SteamAchievementSaveGame* SaveGame = GetOrCreateSaveGame();
    if (!SaveGame)
    {
        return nullptr;
    }

    for (FSOTS_SteamAchievementState& State : SaveGame->AchievementStates)
    {
        if (State.InternalId == InternalId)
        {
            return &State;
        }
    }

    FSOTS_SteamAchievementState NewState;
    NewState.InternalId = InternalId;
    SaveGame->AchievementStates.Add(NewState);

    return &SaveGame->AchievementStates.Last();
}

bool USOTS_SteamAchievementsSubsystem::ShouldUseOnlineAchievements() const
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

    if (!Settings->bEnableAchievements)
    {
        return false;
    }

    if (!Settings->bEnableSteamIntegration || !Settings->bUseSteamForAchievements)
    {
        return false;
    }

    return true;
}

IOnlineSubsystem* USOTS_SteamAchievementsSubsystem::GetOnlineSubsystemSteam() const
{
    return GetOnlineSubsystemSafe();
}

IOnlineIdentityPtr USOTS_SteamAchievementsSubsystem::GetOnlineIdentityInterface() const
{
    IOnlineSubsystem* Subsystem = GetOnlineSubsystemSteam();
    if (!Subsystem)
    {
        return nullptr;
    }

    return Subsystem->GetIdentityInterface();
}

IOnlineAchievementsPtr USOTS_SteamAchievementsSubsystem::GetOnlineAchievementsInterface() const
{
    IOnlineSubsystem* Subsystem = GetOnlineSubsystemSteam();
    if (!Subsystem)
    {
        return nullptr;
    }

    return Subsystem->GetAchievementsInterface();
}

TSharedPtr<const FUniqueNetId> USOTS_SteamAchievementsSubsystem::GetPrimaryUserId() const
{
    IOnlineIdentityPtr Identity = GetOnlineIdentityInterface();
    if (!Identity.IsValid())
    {
        return nullptr;
    }

    return Identity->GetUniquePlayerId(0);
}

bool USOTS_SteamAchievementsSubsystem::TryPushAchievementToOnline(
    FName InternalId,
    const FSOTS_SteamAchievementDefinition* Def,
    const FSOTS_SteamAchievementState* State) const
{
    if (!ShouldUseOnlineAchievements())
    {
        return false;
    }

    if (InternalId.IsNone() || !State)
    {
        return false;
    }

    const FSOTS_SteamAchievementDefinition* ResolvedDef = Def;
    if (!ResolvedDef)
    {
        const USOTS_SteamAchievementRegistry* Registry = const_cast<USOTS_SteamAchievementsSubsystem*>(this)->GetAchievementRegistry();
        if (!Registry)
        {
            return false;
        }

        ResolvedDef = Registry->FindByInternalId(InternalId);
    }

    if (!ResolvedDef)
    {
        return false;
    }

    FName ApiName = !ResolvedDef->SteamApiName.IsNone()
        ? ResolvedDef->SteamApiName
        : ResolvedDef->InternalId;

    if (ApiName.IsNone())
    {
        return false;
    }

    IOnlineAchievementsPtr Achievements = GetOnlineAchievementsInterface();
    if (!Achievements.IsValid())
    {
        return false;
    }

    const TSharedPtr<const FUniqueNetId> UserId = GetPrimaryUserId();
    if (!UserId.IsValid())
    {
        return false;
    }

    FOnlineAchievementsWritePtr WriteObject = MakeShared<FOnlineAchievementsWrite>();

    float Progress = 1.0f;
    if (State->Max > 0)
    {
        Progress = FMath::Clamp<float>(static_cast<float>(State->Current) / static_cast<float>(State->Max), 0.0f, 1.0f);
    }

    WriteObject->SetFloatStat(ApiName.ToString(), Progress);

    FOnlineAchievementsWriteRef WriteRef = WriteObject.ToSharedRef();
    Achievements->WriteAchievements(*UserId, WriteRef);
    return true;
}

bool USOTS_SteamAchievementsSubsystem::IsOnlineAchievementSyncAvailable() const
{
    if (!ShouldUseOnlineAchievements())
    {
        return false;
    }

    if (!GetOnlineAchievementsInterface().IsValid())
    {
        return false;
    }

    if (!GetPrimaryUserId().IsValid())
    {
        return false;
    }

    return true;
}

bool USOTS_SteamAchievementsSubsystem::IsAchievementUnlocked(FName InternalId) const
{
    if (!bStatsLoaded)
    {
        if (USOTS_SteamSettings::IsVerboseLoggingEnabled())
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("IsAchievementUnlocked - Stats not loaded yet."));
        }
        return false;
    }

    USOTS_SteamAchievementsSubsystem* MutableThis = const_cast<USOTS_SteamAchievementsSubsystem*>(this);
    USOTS_SteamAchievementSaveGame* SaveGame = MutableThis->GetOrCreateSaveGame();
    if (!SaveGame)
    {
        return false;
    }

    for (const FSOTS_SteamAchievementState& State : SaveGame->AchievementStates)
    {
        if (State.InternalId == InternalId)
        {
            return State.bUnlocked;
        }
    }

    return false;
}

bool USOTS_SteamAchievementsSubsystem::GetAchievementProgress(FName InternalId, int32& OutCurrent, int32& OutMax) const
{
    OutCurrent = 0;
    OutMax = 0;

    if (!bStatsLoaded)
    {
        if (USOTS_SteamSettings::IsVerboseLoggingEnabled())
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("GetAchievementProgress - Stats not loaded yet."));
        }
        return false;
    }

    USOTS_SteamAchievementsSubsystem* MutableThis = const_cast<USOTS_SteamAchievementsSubsystem*>(this);
    USOTS_SteamAchievementSaveGame* SaveGame = MutableThis->GetOrCreateSaveGame();
    if (!SaveGame)
    {
        return false;
    }

    for (const FSOTS_SteamAchievementState& State : SaveGame->AchievementStates)
    {
        if (State.InternalId == InternalId)
        {
            OutCurrent = State.Current;
            OutMax = State.Max;
            return true;
        }
    }

    return false;
}

bool USOTS_SteamAchievementsSubsystem::SetAchievement(FName InternalId)
{
    if (!bStatsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("SetAchievement - Stats not loaded. Call RequestCurrentStats first."));
        return false;
    }

    const USOTS_SteamAchievementRegistry* Registry = GetAchievementRegistry();
    if (!Registry)
    {
        return false;
    }

    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("SetAchievement - InternalId is None."));
        return false;
    }

    const FSOTS_SteamAchievementDefinition* Def = Registry->FindByInternalId(InternalId);
    if (!Def)
    {
        return false;
    }

    FSOTS_SteamAchievementState* State = FindOrAddState(InternalId);
    if (!State)
    {
        return false;
    }

    if (!State->bUnlocked)
    {
        State->bUnlocked = true;

        if (State->Max > 0)
        {
            State->Current = State->Max;
        }

        OnAchievementUnlocked.Broadcast(InternalId);
        TryPushAchievementToOnline(InternalId, Def, State);
    }

    return true;
}

bool USOTS_SteamAchievementsSubsystem::ClearAchievement(FName InternalId)
{
    if (!bStatsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("ClearAchievement - Stats not loaded. Call RequestCurrentStats first."));
        return false;
    }

    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("ClearAchievement - InternalId is None."));
        return false;
    }

    FSOTS_SteamAchievementState* State = FindOrAddState(InternalId);
    if (!State)
    {
        return false;
    }

    State->bUnlocked = false;
    State->Current = 0;
    State->Max = FMath::Max(State->Max, 0);

    return true;
}

bool USOTS_SteamAchievementsSubsystem::IndicateAchievementProgress(
    FName InternalId,
    int32 Current,
    int32 Max,
    bool bAutoUnlockOnComplete)
{
    if (!bStatsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("IndicateAchievementProgress - Stats not loaded. Call RequestCurrentStats first."));
        return false;
    }

    if (InternalId.IsNone())
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("IndicateAchievementProgress - InternalId is None."));
        return false;
    }

    if (Max < 0 || Current < 0)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("IndicateAchievementProgress - Negative progress values are not allowed."));
        return false;
    }

    FSOTS_SteamAchievementState* State = FindOrAddState(InternalId);
    if (!State)
    {
        return false;
    }

    State->Current = Current;
    State->Max = Max;

    OnAchievementProgress.Broadcast(InternalId, State->Current, State->Max);

    const USOTS_SteamAchievementRegistry* Registry = GetAchievementRegistry();
    const FSOTS_SteamAchievementDefinition* Def = Registry ? Registry->FindByInternalId(InternalId) : nullptr;
    TryPushAchievementToOnline(InternalId, Def, State);

    if (bAutoUnlockOnComplete && State->Max > 0 && State->Current >= State->Max)
    {
        return SetAchievement(InternalId);
    }

    return true;
}

void USOTS_SteamAchievementsSubsystem::DumpAchievementsToLog() const
{
    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();

    UE_LOG(LogSOTS_Steam, Log, TEXT("---- SOTS_Steam Achievements Dump ----"));

    if (!bStatsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Log, TEXT("  Stats not loaded (bStatsLoaded == false)."));
        return;
    }

    if (!CurrentSaveGame)
    {
        UE_LOG(LogSOTS_Steam, Log, TEXT("  CurrentSaveGame is null."));
        return;
    }

    const int32 NumStates = CurrentSaveGame->AchievementStates.Num();
    UE_LOG(LogSOTS_Steam, Log, TEXT("  AchievementStates: %d entries."), NumStates);

    const bool bVerbose = Settings && Settings->bEnableVerboseLogging;

    for (const FSOTS_SteamAchievementState& State : CurrentSaveGame->AchievementStates)
    {
        const FString Status = State.bUnlocked ? TEXT("Unlocked") : TEXT("Locked");

        if (bVerbose)
        {
            UE_LOG(LogSOTS_Steam, Log,
                TEXT("    [%s] %s - Current: %d / Max: %d"),
                *State.InternalId.ToString(),
                *Status,
                State.Current,
                State.Max);
        }
        else
        {
            UE_LOG(LogSOTS_Steam, Log,
                TEXT("    [%s] %s"),
                *State.InternalId.ToString(),
                *Status);
        }
    }

    UE_LOG(LogSOTS_Steam, Log, TEXT("---- End of SOTS_Steam Achievements Dump ----"));
}

FGameplayTagContainer USOTS_SteamAchievementsSubsystem::BuildMissionResultTags(const FSOTS_SteamMissionResult& Result) const
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

void USOTS_SteamAchievementsSubsystem::HandleMissionResult(const FSOTS_SteamMissionResult& Result)
{
    if (!bStatsLoaded)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("HandleMissionResult - Stats not loaded. Call RequestCurrentStats first."));
        return;
    }

    const USOTS_SteamSettings* Settings = USOTS_SteamSettings::Get();
    if (!Settings || !Settings->bEnableAchievements)
    {
        if (USOTS_SteamSettings::IsVerboseLoggingEnabled())
        {
            UE_LOG(LogSOTS_Steam, Verbose, TEXT("HandleMissionResult - Achievements disabled in settings; treating as no-op."));
        }
        return;
    }

    const USOTS_SteamAchievementRegistry* Registry = GetAchievementRegistry();
    if (!Registry)
    {
        UE_LOG(LogSOTS_Steam, Warning, TEXT("HandleMissionResult - No achievement registry available."));
        return;
    }

    const FGameplayTagContainer MissionTags = BuildMissionResultTags(Result);

    for (const FSOTS_SteamAchievementDefinition& Def : Registry->Achievements)
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
            SetAchievement(Def.InternalId);
        }
    }
}
