#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Data/SOTS_AbilityTypes.h"
#include "SOTS_GlobalStealthTypes.h"
#include "SOTS_AbilityBase.generated.h"

class UAC_SOTS_Abilitys;
class AActor;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SOTS_GAS_PLUGIN_API USOTS_AbilityBase : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="SOTS Ability")
    FGameplayTag AbilityTag;

    UPROPERTY(BlueprintReadOnly, Category="SOTS Ability")
    F_SOTS_AbilityHandle Handle;

    UPROPERTY(BlueprintReadOnly, Category="SOTS Ability")
    TObjectPtr<UAC_SOTS_Abilitys> OwningComponent;

    UPROPERTY(BlueprintReadOnly, Category="SOTS Ability")
    TObjectPtr<AActor> OwningActor;

    UFUNCTION(BlueprintCallable, Category="SOTS Ability")
    void Initialize(UAC_SOTS_Abilitys* InComponent,
                    const F_SOTS_AbilityDefinition& InDefinition,
                    const F_SOTS_AbilityHandle& InHandle);

    UFUNCTION(BlueprintImplementableEvent, Category="SOTS Ability")
    void OnAbilityGranted(const F_SOTS_AbilityDefinition& Definition);

    UFUNCTION(BlueprintImplementableEvent, Category="SOTS Ability")
    void OnAbilityRemoved();

    UFUNCTION(BlueprintImplementableEvent, Category="SOTS Ability")
    void K2_ActivateAbility(const F_SOTS_AbilityActivationContext& Context);

    UFUNCTION(BlueprintImplementableEvent, Category="SOTS Ability")
    void K2_EndAbility();

    // ---- Stealth helpers ----

    UFUNCTION(BlueprintPure, Category="SOTS|Stealth")
    float GetOwnerLightLevel01() const;

    UFUNCTION(BlueprintPure, Category="SOTS|Stealth")
    float GetOwnerGlobalStealthScore01() const;

    UFUNCTION(BlueprintPure, Category="SOTS|Stealth")
    ESOTS_StealthTier GetOwnerStealthTier() const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void ApplyStealthModifierToWorld(const FSOTS_StealthModifier& Modifier) const;

    UFUNCTION(BlueprintCallable, Category="SOTS|Stealth")
    void RemoveStealthModifierFromWorld(FName SourceId) const;
};
