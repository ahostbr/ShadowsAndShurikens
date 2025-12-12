#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_GlobalStealthTagLibrary.generated.h"

/**
 * Simple helpers for querying stealth-related tags from the global player tag
 * state. These are thin wrappers around GSM + TagManager and are intended
 * primarily for Blueprint usage.
 */
UCLASS()
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_GlobalStealthTagLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Returns true when the player is effectively hidden, based on either the
    // current GSM tier or the presence of Player.Stealth.Hidden in the global
    // tag container.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_IsPlayerHidden(const UObject* WorldContextObject);

    // Returns the current stealth tier as an integer, using GSM if present
    // and falling back to tier tags on the global tag state.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|Stealth", meta=(WorldContext="WorldContextObject"))
    static int32 SOTS_GetStealthTier(const UObject* WorldContextObject);
};

