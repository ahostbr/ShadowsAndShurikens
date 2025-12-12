#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_GAS_DebugLibrary.generated.h"

class USOTS_AbilityRequirementLibraryAsset;

/**
 * Global stealth/perception debug mode.
 * Off      = no debug widgets or HUD.
 * Basic    = compact world-space widgets + minimal HUD.
 * Advanced = expanded information everywhere.
 */
UENUM(BlueprintType)
enum class ESOTSStealthDebugMode : uint8
{
    Off     UMETA(DisplayName="Off"),
    Basic   UMETA(DisplayName="Basic"),
    Advanced UMETA(DisplayName="Advanced")
};

/**
 * Debug-only helpers for logging current stealth, tags, and
 * ability requirement evaluations. These functions do not
 * change gameplay state; they only write to the log or expose
 * the current debug mode used by UMG widgets.
 */
UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_GAS_DebugLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Global debug mode accessors, backed by a console variable
    // `sots.StealthDebugMode` (0=Off,1=Basic,2=Advanced). In
    // shipping builds this always returns Off and Set is a no-op.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Debug")
    static ESOTSStealthDebugMode GetStealthDebugMode();

    UFUNCTION(BlueprintCallable, Category="SOTS|GAS|Debug")
    static void SetStealthDebugMode(ESOTSStealthDebugMode NewMode);

    UFUNCTION(BlueprintCallable, Category="SOTS|GAS|Debug", meta=(WorldContext="WorldContextObject"))
    static void LogCurrentStealthState(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|GAS|Debug", meta=(WorldContext="WorldContextObject"))
    static void LogCurrentSkillAndPlayerTags(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="SOTS|GAS|Debug", meta=(WorldContext="WorldContextObject"))
    static void LogAbilityRequirementsFromLibrary(
        const UObject* WorldContextObject,
        USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
        FGameplayTag AbilityTag);
};
