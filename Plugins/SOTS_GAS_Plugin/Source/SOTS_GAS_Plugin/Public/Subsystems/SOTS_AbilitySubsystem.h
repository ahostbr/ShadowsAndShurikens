#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SOTS_ProfileTypes.h"
#include "SOTS_AbilitySubsystem.generated.h"

class UWorld;

UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_AbilitySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Subsystem", meta=(WorldContext="WorldContextObject"))
    static USOTS_AbilitySubsystem* Get(const UObject* WorldContextObject);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS Ability|Profile")
    TArray<FGameplayTag> GrantedAbilityTags;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS Ability|Profile")
    TMap<FGameplayTag, int32> AbilityRanks;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SOTS Ability|Profile")
    TMap<FGameplayTag, float> AbilityCooldownRemaining;

    void BuildProfileData(FSOTS_AbilityProfileData& OutData) const;
    void ApplyProfileData(const FSOTS_AbilityProfileData& InData);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Profile")
    void GrantAbility(FGameplayTag AbilityTag, int32 Rank = 1);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Profile")
    void RemoveAbility(FGameplayTag AbilityTag);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Profile")
    bool HasAbility(FGameplayTag AbilityTag) const;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Profile")
    void SetAbilityRank(FGameplayTag AbilityTag, int32 NewRank);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Profile")
    int32 GetAbilityRank(FGameplayTag AbilityTag) const;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability|Profile")
    void SetAbilityCooldownRemaining(FGameplayTag AbilityTag, float RemainingSeconds);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Profile")
    float GetAbilityCooldownRemaining(FGameplayTag AbilityTag) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SOTS Ability|Debug")
    FString GetAbilityProfileSummary() const;
};
