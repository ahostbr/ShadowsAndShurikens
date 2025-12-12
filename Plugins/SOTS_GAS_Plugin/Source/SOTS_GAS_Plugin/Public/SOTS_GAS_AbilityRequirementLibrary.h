#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Data/SOTS_AbilityTypes.h"
#include "SOTS_GAS_AbilityRequirementLibrary.generated.h"

class USOTS_AbilityRequirementLibraryAsset;

USTRUCT(BlueprintType)
struct FSOTS_AbilityRequirementCheckResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bMeetsAllRequirements = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bMissingSkillTags = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bMissingPlayerTags = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bStealthTierTooLow = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bStealthTierTooHigh = false;

    UPROPERTY(BlueprintReadOnly, Category="SOTS|GAS|Ability")
    bool bStealthScoreTooHigh = false;
};

UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_GAS_AbilityRequirementLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // High-level convenience wrapper that resolves requirements from a library,
    // evaluates them, and returns both the structured result and a debug string.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability", meta=(WorldContext="WorldContextObject"))
    static bool EvaluateAbilityFromLibraryWithReason(
        const UObject* WorldContextObject,
        USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
        FGameplayTag AbilityTag,
        FSOTS_AbilityRequirementCheckResult& OutResult,
        FText& OutDebugDescription);

    // Resolve requirements by tag from a DataAsset and evaluate them.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability", meta=(WorldContext="WorldContextObject"))
    static FSOTS_AbilityRequirementCheckResult EvaluateAbilityRequirementsFromLibrary(
        const UObject* WorldContextObject,
        USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
        FGameplayTag AbilityTag);

    // Evaluate the requirements struct directly (no DataAsset).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability", meta=(WorldContext="WorldContextObject"))
    static FSOTS_AbilityRequirementCheckResult EvaluateAbilityRequirements(
        const UObject* WorldContextObject,
        const FSOTS_AbilityRequirements& Requirements);

    // Convenience wrapper that returns a bool and optional failure reason.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability", meta=(WorldContext="WorldContextObject"))
    static bool EvaluateAbilityRequirementsWithReason(
        const UObject* WorldContextObject,
        const FSOTS_AbilityRequirements& Requirements,
        FText& OutFailureReason);

    // Convenience boolean wrapper.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability", meta=(WorldContext="WorldContextObject"))
    static bool CanActivateAbilityWithRequirements(
        const UObject* WorldContextObject,
        const FSOTS_AbilityRequirements& Requirements);

    // High-level helper that looks up requirements by ability tag and reports
    // a localized failure reason. This is intended for ability UI and gating.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability", meta=(WorldContext="WorldContextObject"))
    static bool SOTS_CanActivateAbilityByTag(
        const UObject* WorldContextObject,
        USOTS_AbilityRequirementLibraryAsset* LibraryAsset,
        FGameplayTag AbilityTag,
        FText& OutFailureReason);

    // Human-readable explanation of a requirement check result, suitable for UI/debug.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS|GAS|Ability")
    static FText DescribeAbilityRequirementCheckResult(const FSOTS_AbilityRequirementCheckResult& Result);

private:
    static bool CollectPlayerSkillTags(const UObject* WorldContextObject, FGameplayTagContainer& OutSkillTags);
    static bool CollectPlayerGenericTags(const UObject* WorldContextObject, FGameplayTagContainer& OutPlayerTags);
    static bool GetCurrentStealthState(const UObject* WorldContextObject, int32& OutTier, float& OutScore01);
};
