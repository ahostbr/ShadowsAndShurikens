#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/SOTS_AbilityTypes.h"
#include "SOTS_AbilityBlueprintLibrary.generated.h"

class UAC_SOTS_Abilitys;
class AActor;

UCLASS()
class SOTS_GAS_PLUGIN_API USOTS_AbilityBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="SOTS Ability", meta=(DefaultToSelf="TargetActor"))
    static UAC_SOTS_Abilitys* GetAbilityComponentFromActor(AActor* TargetActor, bool& bOutFound);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability", meta=(DefaultToSelf="TargetActor"))
    static bool SOTS_GrantAbility(AActor* TargetActor, FGameplayTag AbilityTag, const F_SOTS_AbilityGrantOptions& Options, F_SOTS_AbilityHandle& OutHandle);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability", meta=(DefaultToSelf="TargetActor"))
    static bool SOTS_TryActivateAbility(AActor* TargetActor, FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, E_SOTS_AbilityActivationResult& OutResult);

    UFUNCTION(BlueprintCallable, Category="SOTS Ability", meta=(DefaultToSelf="TargetActor"))
    static void SOTS_CancelAllAbilities(AActor* TargetActor);
};
