#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamLeaderboardSaveGame.generated.h"

USTRUCT(BlueprintType)
struct SOTS_STEAM_API FSOTS_SteamLeaderboardBoardState
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    FName InternalId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    int32 BestScore = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    int32 LastScore = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboard")
    FString PlayerName;
};

UCLASS()
class SOTS_STEAM_API USOTS_SteamLeaderboardSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Leaderboards")
    TArray<FSOTS_SteamLeaderboardBoardState> LeaderboardStates;
};
