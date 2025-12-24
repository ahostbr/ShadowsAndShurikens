#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_StatsLibrary.generated.h"

class AActor;
class USOTS_StatsComponent;

UCLASS()
class SOTS_STATS_API USOTS_StatsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static float GetActorStatValue(const UObject* WorldContextObject, AActor* Target, FGameplayTag StatTag, float DefaultValue = 0.f);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static void AddToActorStat(const UObject* WorldContextObject, AActor* Target, FGameplayTag StatTag, float Delta);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static void SetActorStatValue(const UObject* WorldContextObject, AActor* Target, FGameplayTag StatTag, float NewValue);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static void SnapshotActorStats(AActor* Target, FSOTS_CharacterStateData& OutState);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static void ApplySnapshotToActor(AActor* Target, const FSOTS_CharacterStateData& Snapshot);

    /** Resolve-or-create stats component on the actor (per-actor isolation) and return it. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static USOTS_StatsComponent* ResolveStatsComponentForActor(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static void SOTS_BuildCharacterStateFromStats(const UObject* WorldContextObject, AActor* Target, FSOTS_CharacterStateData& OutState);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats", meta = (WorldContext = "WorldContextObject"))
    static void SOTS_ApplyCharacterStateToStats(const UObject* WorldContextObject, AActor* Target, const FSOTS_CharacterStateData& InState);

    UFUNCTION(BlueprintCallable, Category = "SOTS|Stats|DevTools", meta = (WorldContext = "WorldContextObject"))
    static FString SOTS_DumpStatsToString(const UObject* WorldContextObject, AActor* Target);
};
