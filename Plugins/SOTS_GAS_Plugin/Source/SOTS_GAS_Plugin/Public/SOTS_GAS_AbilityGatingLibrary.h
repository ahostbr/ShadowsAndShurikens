#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_GAS_AbilityGatingLibrary.generated.h"

class USOTS_GAS_StealthBridgeSubsystem;

/**
 * Lightweight, read-only helpers for gating abilities based on
 * skill tags, global player tags, and current stealth tier.
 */
UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_GAS_AbilityGatingLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Returns true if the player currently has all of the specified skill tags
    // granted by the skill tree subsystem. Empty Tags is treated as trivially true.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability|Gating", meta=(WorldContext="WorldContextObject"))
    static bool PlayerHasAllSkillTags(const UObject* WorldContextObject, const FGameplayTagContainer& Tags);

    // Returns true if the player currently has at least one of the specified
    // skill tags. Empty Tags returns false.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability|Gating", meta=(WorldContext="WorldContextObject"))
    static bool PlayerHasAnySkillTag(const UObject* WorldContextObject, const FGameplayTagContainer& Tags);

    // Returns true if the global player tag state contains the given tag.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability|Gating", meta=(WorldContext="WorldContextObject"))
    static bool PlayerHasGlobalTag(const UObject* WorldContextObject, FGameplayTag Tag);

    // Returns true if the current stealth tier (from GSM via the GAS stealth bridge)
    // is less than or equal to MaxTierInclusive.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability|Gating", meta=(WorldContext="WorldContextObject"))
    static bool IsStealthTierAtOrBelow(const UObject* WorldContextObject, int32 MaxTierInclusive);

    // Returns true if the current stealth tier lies within [MinTierInclusive, MaxTierInclusive].
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability|Gating", meta=(WorldContext="WorldContextObject"))
    static bool IsStealthTierWithinRange(const UObject* WorldContextObject, int32 MinTierInclusive, int32 MaxTierInclusive);
};

