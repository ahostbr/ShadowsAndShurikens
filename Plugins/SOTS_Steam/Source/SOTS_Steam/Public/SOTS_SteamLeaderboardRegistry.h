#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamLeaderboardRegistry.generated.h"

UCLASS(BlueprintType)
class SOTS_STEAM_API USOTS_SteamLeaderboardRegistry : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Leaderboards")
    TArray<FSOTS_SteamLeaderboardDefinition> Leaderboards;

    const FSOTS_SteamLeaderboardDefinition* FindByInternalId(FName InternalId) const;
};
