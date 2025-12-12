#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SOTS_SteamTypes.generated.h"

USTRUCT(BlueprintType)
struct SOTS_STEAM_API FSOTS_SteamAchievementDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievement")
    FName InternalId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievement")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievement")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievement")
    FName SteamApiName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievement")
    FGameplayTagContainer Tags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievement")
    bool bIsSecret = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievement")
    int32 SortOrder = 0;
};

USTRUCT(BlueprintType)
struct SOTS_STEAM_API FSOTS_SteamLeaderboardDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leaderboard")
    FName InternalId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leaderboard")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leaderboard")
    bool bHigherIsBetter = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leaderboard")
    FGameplayTagContainer Tags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leaderboard")
    int32 SortOrder = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Steam")
    FName SteamApiName;

    // Optional explicit rated stat name when writing to Steam. Falls back to SteamApiName or InternalId.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Steam")
    FName RatedStatName;

    // Mirror this leaderboard to Steam via OnlineSubsystemSteam when enabled.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Steam")
    bool bMirrorToSteam = true;
};

USTRUCT(BlueprintType)
struct SOTS_STEAM_API FSOTS_SteamLeaderboardRow
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    FName InternalId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    int32 Rank = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    FString PlayerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    int32 Score = 0;
};

USTRUCT(BlueprintType)
struct SOTS_STEAM_API FSOTS_SteamMissionResult
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    FGameplayTag MissionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    FGameplayTag DifficultyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    int32 Score = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    float MissionTimeSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    bool bNoKills = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    bool bNoAlerts = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    bool bPerfectStealth = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mission")
    FGameplayTagContainer AdditionalTags;
};
