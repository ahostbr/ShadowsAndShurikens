#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamAchievementSaveGame.generated.h"

USTRUCT(BlueprintType)
struct SOTS_STEAM_API FSOTS_SteamAchievementState
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Achievement")
    FName InternalId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Achievement")
    bool bUnlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Achievement")
    int32 Current = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Achievement")
    int32 Max = 0;
};

UCLASS()
class SOTS_STEAM_API USOTS_SteamAchievementSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Achievements")
    TArray<FSOTS_SteamAchievementState> AchievementStates;
};
