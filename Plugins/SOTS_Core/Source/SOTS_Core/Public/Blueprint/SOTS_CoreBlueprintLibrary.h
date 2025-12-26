#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/SOTS_CoreLifecycleSubsystem.h"
#include "SOTS_CoreBlueprintLibrary.generated.h"

class UObject;
class APlayerController;
class APawn;

UCLASS()
class SOTS_CORE_API USOTS_CoreBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "SOTS|Core", meta = (WorldContext = "WorldContextObject"))
    static USOTS_CoreLifecycleSubsystem* GetCoreLifecycle(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "SOTS|Core", meta = (WorldContext = "WorldContextObject"))
    static FSOTS_CoreLifecycleSnapshot GetCoreLifecycleSnapshot(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "SOTS|Core", meta = (WorldContext = "WorldContextObject"))
    static FSOTS_PrimaryPlayerIdentity GetPrimaryPlayerIdentity(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "SOTS|Core", meta = (WorldContext = "WorldContextObject"))
    static bool IsPrimaryPlayerReady(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "SOTS|Core", meta = (WorldContext = "WorldContextObject"))
    static APlayerController* GetPrimaryPlayerController(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "SOTS|Core", meta = (WorldContext = "WorldContextObject"))
    static APawn* GetPrimaryPawn(const UObject* WorldContextObject);
};
