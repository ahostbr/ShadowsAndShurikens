#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTSFXTypes.h"
#include "SOTS_FXBlueprintLibrary.generated.h"

class USOTS_FXManagerSubsystem;
class USOTS_FXCueDefinition;

UCLASS()
class SOTS_FX_PLUGIN_API USOTS_FXBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintPure, Category = "SOTS|FX", meta = (DisplayName="Get SOTS FXManager Subsystem"))
    static USOTS_FXManagerSubsystem* GetFXManager();

    UFUNCTION(BlueprintCallable, Category = "SOTS|FX", meta = (DisplayName="Play FX Cue By Tag"))
    static FSOTS_FXHandle PlayFXCueByTag(FGameplayTag CueTag, const FSOTS_FXContext& Context);

    UFUNCTION(BlueprintCallable, Category = "SOTS|FX", meta = (DisplayName="Play FX Cue Definition"))
    static FSOTS_FXHandle PlayFXCueDefinition(USOTS_FXCueDefinition* CueDefinition, const FSOTS_FXContext& Context);

    /** Stop (and optionally immediately kill) an FX handle. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|FX", meta=(DisplayName="Stop FX Handle"))
    static void StopFXHandle(const FSOTS_FXHandle& Handle, bool bImmediate);

    /** Tag-driven FX trigger that returns a detailed result payload. */
    UFUNCTION(BlueprintCallable, Category = "SOTS|FX", meta=(DisplayName="Trigger FX By Tag (With Report)", WorldContext="WorldContextObject"))
    static FSOTS_FXRequestResult TriggerFXWithReport(UObject* WorldContextObject, const FSOTS_FXRequest& Request);

    /** Debug helper: get counts of currently active Niagara & Audio FX. */
    UFUNCTION(BlueprintPure, Category = "SOTS|FX|Debug", meta=(DisplayName="Get FX Active Counts"))
    static FSOTS_FXActiveCounts GetFXActiveCounts();

    /** Debug helper: list currently registered FX cues (tags + assets). */
    UFUNCTION(BlueprintPure, Category = "SOTS|FX|Debug", meta=(DisplayName="Get Registered FX Cues"))
    static void GetRegisteredFXCues(TArray<FGameplayTag>& OutCueTags, TArray<USOTS_FXCueDefinition*>& OutCueDefinitions);
};
