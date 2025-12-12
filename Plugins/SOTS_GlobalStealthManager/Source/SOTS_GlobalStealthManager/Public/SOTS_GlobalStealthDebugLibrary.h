#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SOTS_GlobalStealthDebugLibrary.generated.h"

class AActor;

/**
 * Debug helpers for quickly verifying Global Stealth Manager behavior
 * using real in-world data (player velocity, crouch state, nearest enemy).
 */
UCLASS()
class SOTS_GLOBALSTEALTHMANAGER_API USOTS_GlobalStealthDebugLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Builds a stealth sample from the player and nearest enemy and sends it
     * to the Global Stealth Manager. Results are printed to screen and log.
     *
     * - Noise is based on player velocity.
     * - "Cover" is approximated by the character's crouch state.
     * - Light exposure is a simple value derived from crouch/not-crouched.
     * - Distance and line-of-sight are calculated using the nearest enemy of EnemyClass.
     */
    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth|Debug", meta=(WorldContext="WorldContextObject"))
    static void RunSOTS_GSM_DebugSample(
        const UObject* WorldContextObject,
        AActor* PlayerActor,
        TSubclassOf<AActor> EnemyClass);
};