#pragma once

#include "CoreMinimal.h"
#include "SOTS_SteamSubsystemBase.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamAchievementRegistry.h"
#include "SOTS_SteamAchievementSaveGame.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "SOTS_SteamAchievementsSubsystem.generated.h"

class USOTS_SteamAchievementSaveGame;
class IOnlineSubsystem;
class FUniqueNetId;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSOTS_OnAchievementUnlocked, FName, InternalId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSOTS_OnAchievementProgress,
    FName, InternalId,
    int32, Current,
    int32, Max);

UCLASS()
class SOTS_STEAM_API USOTS_SteamAchievementsSubsystem : public USOTS_SteamSubsystemBase
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    void SetCurrentProfileId(const FString& InProfileId);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool RequestCurrentStats();

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool StoreStats();

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool AreStatsLoaded() const { return bStatsLoaded; }

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool IsAchievementUnlocked(FName InternalId) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool GetAchievementProgress(FName InternalId, int32& OutCurrent, int32& OutMax) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool SetAchievement(FName InternalId);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool ClearAchievement(FName InternalId);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    bool IndicateAchievementProgress(
        FName InternalId,
        int32 Current,
        int32 Max,
        bool bAutoUnlockOnComplete);

    UFUNCTION(BlueprintPure, Category="SOTS|Steam|Achievements")
    bool IsOnlineAchievementSyncAvailable() const;

    // Debug helper: dump current local achievement state to the log.
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements|Debug")
    void DumpAchievementsToLog() const;

    // -------------------------
    // Mission result integration
    // -------------------------

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Achievements")
    void HandleMissionResult(const FSOTS_SteamMissionResult& Result);

public:
    UPROPERTY(BlueprintAssignable, Category="SOTS|Steam|Achievements")
    FSOTS_OnAchievementUnlocked OnAchievementUnlocked;

    UPROPERTY(BlueprintAssignable, Category="SOTS|Steam|Achievements")
    FSOTS_OnAchievementProgress OnAchievementProgress;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SOTS|Steam|Achievements", meta=(AllowPrivateAccess="true"))
    TSoftObjectPtr<USOTS_SteamAchievementRegistry> AchievementRegistryOverride;

private:
    const USOTS_SteamAchievementRegistry* GetAchievementRegistry();
    FString GetSaveSlotName() const;
    USOTS_SteamAchievementSaveGame* GetOrCreateSaveGame();
    FSOTS_SteamAchievementState* FindOrAddState(FName InternalId);
    FGameplayTagContainer BuildMissionResultTags(const FSOTS_SteamMissionResult& Result) const;
    bool ShouldUseOnlineAchievements() const;
    IOnlineSubsystem* GetOnlineSubsystemSteam() const;
    IOnlineIdentityPtr GetOnlineIdentityInterface() const;
    IOnlineAchievementsPtr GetOnlineAchievementsInterface() const;
    TSharedPtr<const FUniqueNetId> GetPrimaryUserId() const;
    bool TryPushAchievementToOnline(
        FName InternalId,
        const FSOTS_SteamAchievementDefinition* Def,
        const FSOTS_SteamAchievementState* State) const;

private:
    TWeakObjectPtr<USOTS_SteamAchievementRegistry> CachedRegistry;

    UPROPERTY()
    USOTS_SteamAchievementSaveGame* CurrentSaveGame = nullptr;

    UPROPERTY()
    FString CurrentProfileId;

    UPROPERTY()
    bool bStatsLoaded = false;
};
