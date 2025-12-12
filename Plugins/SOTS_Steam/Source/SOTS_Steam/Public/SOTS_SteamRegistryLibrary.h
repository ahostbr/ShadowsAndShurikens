#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_SteamTypes.h"
#include "SOTS_SteamAchievementRegistry.h"
#include "SOTS_SteamLeaderboardRegistry.h"
#include "SOTS_SteamRegistryLibrary.generated.h"

UCLASS()
class SOTS_STEAM_API USOTS_SteamRegistryLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Registry")
    static bool FindAchievementDefinitionById(
        const USOTS_SteamAchievementRegistry* Registry,
        FName InternalId,
        FSOTS_SteamAchievementDefinition& OutDefinition);

    UFUNCTION(BlueprintCallable, Category="SOTS|Steam|Registry")
    static bool FindLeaderboardDefinitionById(
        const USOTS_SteamLeaderboardRegistry* Registry,
        FName InternalId,
        FSOTS_SteamLeaderboardDefinition& OutDefinition);
};
