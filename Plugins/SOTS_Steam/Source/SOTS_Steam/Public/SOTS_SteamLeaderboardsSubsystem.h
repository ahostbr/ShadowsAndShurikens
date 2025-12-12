#pragma once

#include "CoreMinimal.h"
#include "SOTS_SteamSubsystemBase.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamLeaderboardRegistry.h"
#include "SOTS_SteamLeaderboardSaveGame.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "UObject/SoftObjectPtr.h"
#include "SOTS_SteamLeaderboardsSubsystem.generated.h"

class USOTS_SteamLeaderboardSaveGame;
class IOnlineSubsystem;
class FUniqueNetId;
struct FSOTS_SteamLeaderboardDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSOTS_OnLeaderboardScoreSubmitted,
    FName, InternalId,
    int32, NewScore,
    bool, bIsNewBest);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSOTS_OnLeaderboardUpdated,
    FName, InternalId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSOTS_OnSteamLeaderboardRowsReceived,
    FName, InternalId,
    const TArray<FSOTS_SteamLeaderboardRow>&, Entries,
    bool, bWasSuccessful);

UCLASS()
class SOTS_STEAM_API USOTS_SteamLeaderboardsSubsystem : public USOTS_SteamSubsystemBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    void SetCurrentProfileId(const FString& InProfileId);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool RequestLeaderboardData();

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool StoreLeaderboardData();

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool AreLeaderboardsLoaded() const { return bLeaderboardsLoaded; }

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool SubmitScore(
        FName InternalId,
        int32 Score,
        const FString& PlayerName,
        bool bOnlyIfBetter);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool GetBestScore(FName InternalId, int32& OutScore) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool GetTopEntries(
        FName InternalId,
        int32 MaxEntries,
        TArray<FSOTS_SteamLeaderboardRow>& OutEntries) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool GetAroundPlayer(
        FName InternalId,
        int32 Range,
        TArray<FSOTS_SteamLeaderboardRow>& OutEntries) const;

    // -------------------------
    // Steam leaderboard queries
    // -------------------------

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards|Steam")
    void QuerySteamTopEntries(FName InternalId, int32 NumEntries);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards|Steam")
    void QuerySteamAroundPlayer(FName InternalId, int32 Range);

    UPROPERTY(BlueprintAssignable, Category="SOTS|Steam|Leaderboards|Steam")
    FSOTS_OnSteamLeaderboardRowsReceived OnSteamTopEntriesReceived;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Steam|Leaderboards|Steam")
    FSOTS_OnSteamLeaderboardRowsReceived OnSteamAroundPlayerEntriesReceived;

    // -------------------------
    // Mission result integration
    // -------------------------

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards")
    bool SubmitMissionResultToLeaderboards(
        const FSOTS_SteamMissionResult& Result,
        const FString& PlayerName,
        bool bOnlyIfBetter);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Leaderboards|Debug")
    void DumpLeaderboardsToLog() const;

public:
    UPROPERTY(BlueprintAssignable, Category="SOTS|Steam|Leaderboards")
    FSOTS_OnLeaderboardScoreSubmitted OnScoreSubmitted;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Steam|Leaderboards")
    FSOTS_OnLeaderboardUpdated OnLeaderboardUpdated;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Steam|Leaderboards", meta=(AllowPrivateAccess="true"))
    TSoftObjectPtr<USOTS_SteamLeaderboardRegistry> LeaderboardRegistryOverride;

private:
    const USOTS_SteamLeaderboardRegistry* GetLeaderboardRegistry();
    FString GetSaveSlotName() const;
    USOTS_SteamLeaderboardSaveGame* GetOrCreateSaveGame();
    FSOTS_SteamLeaderboardBoardState* FindOrAddBoardState(FName InternalId);
    bool IsBetterScore(FName InternalId, int32 Candidate, int32 Existing) const;
    FGameplayTagContainer BuildMissionResultTags(const FSOTS_SteamMissionResult& Result) const;
    bool ShouldUseOnlineLeaderboards() const;
    IOnlineSubsystem* GetOnlineSubsystemSteam() const;
    IOnlineIdentityPtr GetOnlineIdentityInterface() const;
    IOnlineLeaderboardsPtr GetOnlineLeaderboardsInterface() const;
    TSharedPtr<const FUniqueNetId> GetPrimaryUserId() const;
    bool TryPushScoreToOnline(
        FName InternalId,
        int32 Score,
        const FSOTS_SteamLeaderboardDefinition* Def,
        bool bOnlyIfBetter) const;

    enum class ESOTS_SteamLeaderboardQueryKind : uint8
    {
        None,
        Top,
        AroundPlayer
    };

    void HandleSteamLeaderboardReadComplete(bool bWasSuccessful);
    void BuildRowsFromCurrentRead(TArray<FSOTS_SteamLeaderboardRow>& OutEntries) const;

    ESOTS_SteamLeaderboardQueryKind CurrentQueryKind = ESOTS_SteamLeaderboardQueryKind::None;
    FName CurrentReadInternalId = NAME_None;
    FName CurrentReadRatedStatName = NAME_None;
    FOnlineLeaderboardReadPtr CurrentReadObject;
    FDelegateHandle LeaderboardReadCompleteDelegateHandle;

private:
    TWeakObjectPtr<USOTS_SteamLeaderboardRegistry> CachedRegistry;

    UPROPERTY()
    USOTS_SteamLeaderboardSaveGame* CurrentSaveGame = nullptr;

    UPROPERTY()
    FString CurrentProfileId;

    UPROPERTY()
    bool bLeaderboardsLoaded = false;
};
