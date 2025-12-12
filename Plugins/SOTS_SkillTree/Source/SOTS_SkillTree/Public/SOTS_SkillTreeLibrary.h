#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SOTS_SkillTreeLibrary.generated.h"

class USOTS_SkillTreeSubsystem;

/**
 * Lightweight Blueprint helpers for querying the global skill tree state.
 * This is intentionally backend-only; it does not modify unlock state.
 */
UCLASS()
class SOTS_SKILLTREE_API USOTS_SkillTreeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree", meta=(WorldContext="WorldContextObject"))
    static bool IsSkillUnlocked(const UObject* WorldContextObject, const FGameplayTag& SkillTag);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree", meta=(WorldContext="WorldContextObject"))
    static void GetAllSkillTags(const UObject* WorldContextObject, FGameplayTagContainer& OutSkillTags);

    // Convenience helper used by GAS/MissionDirector to ask whether the
    // player currently has a specific skill tag. Internally this now
    // delegates to USOTS_SkillTreeSubsystem / the central tag manager.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|SkillTree", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_PlayerHasSkillTag(const UObject* WorldContextObject, FGameplayTag SkillTag);

private:
    static USOTS_SkillTreeSubsystem* GetSkillTreeSubsystem(const UObject* WorldContextObject);
};
