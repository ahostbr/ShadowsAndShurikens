#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/SOTS_AbilityTypes.h"
#include "Data/SOTS_AbilityDataAssets.h"
#include "SOTS_GAS_AbilityRequirementLibrary.h"
#include "SOTS_AbilityRegistrySubsystem.generated.h"

UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_AbilityRegistrySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinition(const F_SOTS_AbilityDefinition& Definition);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinitionDA(USOTS_AbilityDefinitionDA* DefinitionDA);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinitionsFromArray(const TArray<USOTS_AbilityDefinitionDA*>& Definitions);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinitionsFromDataAsset(USOTS_AbilityDefinitionLibrary* Library);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    bool GetAbilityDefinitionByTag(FGameplayTag AbilityTag, F_SOTS_AbilityDefinition& OutDefinition) const;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    bool GetAbilityDefinitionDAByTag(FGameplayTag AbilityTag, USOTS_AbilityDefinitionDA*& OutDefinitionDA) const;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void GetAllAbilityDefinitions(TArray<F_SOTS_AbilityDefinition>& OutDefinitions) const;

    // -------- Requirement helpers --------

    // Evaluate authored requirements for an ability definition by tag. If no
    // definition or no authored requirements are found, the result is treated
    // as "all requirements met" but a warning is logged for missing defs.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Registry", meta=(WorldContext="WorldContextObject"))
    bool EvaluateAbilityRequirementsForTag(const UObject* WorldContextObject,
                                           FGameplayTag AbilityTag,
                                           FSOTS_AbilityRequirementCheckResult& OutResult) const;

    // Convenience wrapper that returns a bool (meets all requirements) and a
    // localized failure reason suitable for UI.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Registry", meta=(WorldContext="WorldContextObject"))
    bool CanActivateAbilityByTag(const UObject* WorldContextObject,
                                  FGameplayTag AbilityTag,
                                  FText& OutFailureReason) const;

protected:
    UPROPERTY()
    TMap<FGameplayTag, F_SOTS_AbilityDefinition> AbilityDefinitions;

    UPROPERTY()
    TMap<FGameplayTag, USOTS_AbilityDefinitionDA*> AbilityDefinitionAssets;
};
