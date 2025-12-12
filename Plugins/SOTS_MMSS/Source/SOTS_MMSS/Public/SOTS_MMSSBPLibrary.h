#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_MMSSBPLibrary.generated.h"

class USOTS_MMSSSubsystem;

/**
 * Lightweight Blueprint helper for accessing the music manager subsystem.
 */
UCLASS()
class SOTS_MMSS_API USOTS_MMSSBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SOTS|Music", meta = (WorldContext = "WorldContextObject"))
    static USOTS_MMSSSubsystem* GetMusicManagerSubsystem(const UObject* WorldContextObject);
};

