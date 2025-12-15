#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_GSM_BlueprintLibrary.generated.h"

UCLASS()
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_GSM_BlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth", meta=(WorldContext="WorldContextObject"))
    static float SOTS_GetStealthScore(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth", meta=(WorldContext="WorldContextObject"))
    static ESOTS_StealthTier SOTS_GetStealthTier(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth", meta=(WorldContext="WorldContextObject"))
    static FGameplayTag SOTS_GetStealthTierTag(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_IsStealthTierAtLeast(const UObject* WorldContextObject, ESOTS_StealthTier Tier);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth", meta=(WorldContext="WorldContextObject"))
    static FGameplayTag SOTS_GetLastStealthReasonTag(const UObject* WorldContextObject);
};
