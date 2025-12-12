#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_GAS_StealthConditionsLibrary.generated.h"

class USOTS_GAS_StealthBridgeSubsystem;

UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_GAS_StealthConditionsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth", meta=(WorldContext="WorldContextObject"))
    static bool IsPlayerHidden(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth", meta=(WorldContext="WorldContextObject"))
    static bool IsPlayerInLowProfileStealth(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth", meta=(WorldContext="WorldContextObject"))
    static bool CanUseLoudAbility(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth", meta=(WorldContext="WorldContextObject"))
    static int32 GetCurrentStealthTier(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Stealth", meta=(WorldContext="WorldContextObject"))
    static float GetCurrentStealthScore01(const UObject* WorldContextObject);

private:
    static USOTS_GAS_StealthBridgeSubsystem* GetBridge(const UObject* WorldContextObject);
};

