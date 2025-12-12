#include "Blueprint/SOTS_AbilityBlueprintLibrary.h"

#include "Components/SOTS_AbilityComponent.h"
#include "GameFramework/Actor.h"

UAC_SOTS_Abilitys* USOTS_AbilityBlueprintLibrary::GetAbilityComponentFromActor(AActor* TargetActor, bool& bOutFound)
{
    bOutFound = false;
    if (!TargetActor)
    {
        return nullptr;
    }

    if (UAC_SOTS_Abilitys* Comp = TargetActor->FindComponentByClass<UAC_SOTS_Abilitys>())
    {
        bOutFound = true;
        return Comp;
    }

    return nullptr;
}

bool USOTS_AbilityBlueprintLibrary::SOTS_GrantAbility(AActor* TargetActor, FGameplayTag AbilityTag, const F_SOTS_AbilityGrantOptions& Options, F_SOTS_AbilityHandle& OutHandle)
{
    OutHandle = F_SOTS_AbilityHandle();

    bool bFound = false;
    if (UAC_SOTS_Abilitys* Comp = GetAbilityComponentFromActor(TargetActor, bFound))
    {
        return Comp->GrantAbility(AbilityTag, Options, OutHandle);
    }

    return false;
}

bool USOTS_AbilityBlueprintLibrary::SOTS_TryActivateAbility(AActor* TargetActor, FGameplayTag AbilityTag, const F_SOTS_AbilityActivationContext& Context, E_SOTS_AbilityActivationResult& OutResult)
{
    OutResult = E_SOTS_AbilityActivationResult::Failed_CustomCondition;

    bool bFound = false;
    if (UAC_SOTS_Abilitys* Comp = GetAbilityComponentFromActor(TargetActor, bFound))
    {
        return Comp->TryActivateAbilityByTag(AbilityTag, Context, OutResult);
    }

    return false;
}

void USOTS_AbilityBlueprintLibrary::SOTS_CancelAllAbilities(AActor* TargetActor)
{
    bool bFound = false;
    if (UAC_SOTS_Abilitys* Comp = GetAbilityComponentFromActor(TargetActor, bFound))
    {
        Comp->CancelAllAbilities();
    }
}
