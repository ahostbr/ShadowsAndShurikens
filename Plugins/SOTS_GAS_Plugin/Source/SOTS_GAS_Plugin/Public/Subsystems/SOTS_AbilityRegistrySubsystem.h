#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/SOTS_AbilityTypes.h"
#include "Data/SOTS_AbilityDataAssets.h"
#include "SOTS_GAS_AbilityRequirementLibrary.h"
#include "SOTS_AbilityRegistrySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilityRegistryReady);

UCLASS(Config=Game)
class SOTS_GAS_PLUGIN_API USOTS_AbilityRegistrySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinition(const F_SOTS_AbilityDefinition& Definition);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    bool RegisterAbilityDefinition_WithResult(const F_SOTS_AbilityDefinition& Definition);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinitionDA(USOTS_AbilityDefinitionDA* DefinitionDA);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    bool RegisterAbilityDefinitionDA_WithResult(USOTS_AbilityDefinitionDA* DefinitionDA);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinitionsFromArray(const TArray<USOTS_AbilityDefinitionDA*>& Definitions);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    int32 RegisterAbilityDefinitionsFromArray_WithResult(const TArray<USOTS_AbilityDefinitionDA*>& Definitions);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    void RegisterAbilityDefinitionsFromDataAsset(USOTS_AbilityDefinitionLibrary* Library);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Registry")
    int32 RegisterAbilityDefinitionsFromDataAsset_WithResult(USOTS_AbilityDefinitionLibrary* Library);

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

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Registry")
    bool IsAbilityRegistryReady() const { return bRegistryReady; }

    /** Broadcast when the registry finishes building. */
    UPROPERTY(BlueprintAssignable, Category="SOTS Ability|Registry")
    FOnAbilityRegistryReady OnAbilityRegistryReady;

protected:
    /** Source-of-truth ability libraries loaded on init (ordered). */
    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry", meta=(AllowedClasses="/Script/SOTS_GAS_Plugin.SOTS_AbilityDefinitionLibrary"))
    TArray<TSoftObjectPtr<USOTS_AbilityDefinitionLibrary>> AbilityDefinitionLibraries;

    /** Optional individual ability definitions loaded on init. */
    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry", meta=(AllowedClasses="/Script/SOTS_GAS_Plugin.SOTS_AbilityDefinitionDA"))
    TArray<TSoftObjectPtr<USOTS_AbilityDefinitionDA>> AdditionalAbilityDefinitions;

    /** Dev-only validation toggles (default OFF). */
    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry|Validation")
    bool bValidateAbilityRegistryOnInit = false;

    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry|Validation")
    bool bLogRegistryLifecycle = false;

    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry|Validation")
    bool bWarnOnDuplicateAbilityTags = false;

    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry|Validation")
    bool bWarnOnMissingAbilityAssets = false;

    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry|Validation")
    bool bWarnOnInvalidAbilityDefs = false;

    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry|Validation")
    bool bWarnOnMissingAbilityTags = false;

    UPROPERTY(EditAnywhere, Config, Category="SOTS Ability|Registry|Validation")
    bool bWarnOnRegistryNotReady = false;

    UPROPERTY()
    TMap<FGameplayTag, F_SOTS_AbilityDefinition> AbilityDefinitions;

    UPROPERTY()
    TMap<FGameplayTag, USOTS_AbilityDefinitionDA*> AbilityDefinitionAssets;

    UPROPERTY()
    bool bRegistryReady = false;

    bool IsRegistryReadyChecked() const;
    void MaybeWarnRegistryNotReady() const;
    void MaybeWarnMissingAbilityTag(FGameplayTag AbilityTag) const;
    void BuildRegistryFromConfig();
    void ValidateRegistry() const;
    void ValidateDefinition(const F_SOTS_AbilityDefinition& Definition, TSet<FGameplayTag>& SeenTags) const;
};
