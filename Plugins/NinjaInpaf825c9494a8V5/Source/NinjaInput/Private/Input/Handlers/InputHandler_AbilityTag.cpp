// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_AbilityTag.h"

#include "AbilitySystemComponent.h"
#include "NinjaInputHandlerHelpers.h"
#include "Components/NinjaInputManagerComponent.h"

UInputHandler_AbilityTag::UInputHandler_AbilityTag()
{
	AbilityTags = FGameplayTagContainer::EmptyContainer;
	CancelFilterTags = FGameplayTagContainer::EmptyContainer;
}

bool UInputHandler_AbilityTag::CanHandle_Implementation(const ETriggerEvent& TriggerEvent,
	const UInputAction* InputAction) const
{
	const bool bHasTags = ensureMsgf(AbilityTags.IsEmpty() == false, TEXT("Please ensure to fill the Ability Tags!"));
	return Super::CanHandle_Implementation(TriggerEvent, InputAction) && bHasTags;
}

UGameplayAbility* UInputHandler_AbilityTag::GetAbilityInstance(const UNinjaInputManagerComponent* Manager) const
{
	const TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = Manager->GetAbilitySystemComponent();

	if (ensureMsgf(IsValid(AbilitySystemComponent), TEXT("No ASC received from the Input Manager.")) &&
		 ensureMsgf(AbilityTags.IsValid(), TEXT("The Gameplay Tag Container must be valid.")))
	{
		TArray<FGameplayAbilitySpecHandle> Handles;
		AbilitySystemComponent->FindAllAbilitiesWithTags(Handles, AbilityTags);

		for (const FGameplayAbilitySpecHandle& Handle : Handles)
		{
			const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
			if (Spec != nullptr && Spec->Handle.IsValid())
			{
				UGameplayAbility* Ability = Spec->GetPrimaryInstance(); 
				if (Ability == nullptr)
				{
					Ability = Spec->Ability;
				}

				return Ability;
			}
		}
	}

	return nullptr;
}

bool UInputHandler_AbilityTag::ActivateAbility(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
	const UInputAction* InputAction) const
{
    UAbilitySystemComponent* AbilitySystemComponent = Manager->GetAbilitySystemComponent();
    if (ensure(IsValid(AbilitySystemComponent)))
    {
        UE_LOG(LogNinjaInputHandler, Verbose, TEXT("[%s] Action %s is triggering abilities with tags %s."),
            *GetNameSafe(Manager->GetOwner()), *GetNameSafe(InputAction), *AbilityTags.ToStringSimple());

        return AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);
    }
	
	return false;
}

bool UInputHandler_AbilityTag::CancelAbility(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
    const UInputAction* InputAction) const
{
    return FNinjaInputHandlerHelpers::InterruptAbilityByTags(Manager, InputAction, AbilityTags, CancelFilterTags);
}