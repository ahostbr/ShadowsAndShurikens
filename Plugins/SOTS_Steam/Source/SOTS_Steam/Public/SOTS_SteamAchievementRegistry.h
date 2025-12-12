#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamAchievementRegistry.generated.h"

UCLASS(BlueprintType)
class SOTS_STEAM_API USOTS_SteamAchievementRegistry : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Achievements")
    TArray<FSOTS_SteamAchievementDefinition> Achievements;

    const FSOTS_SteamAchievementDefinition* FindByInternalId(FName InternalId) const;
};
