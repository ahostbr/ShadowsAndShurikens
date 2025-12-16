// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_AbilityInput.h"

#include "AbilitySystemComponent.h"
#include "NinjaInputHandlerHelpers.h"
#include "Components/NinjaInputManagerComponent.h"

UInputHandler_AbilityInput::UInputHandler_AbilityInput()
{
	InputID = INDEX_NONE;
}

bool UInputHandler_AbilityInput::CanHandle_Implementation(const ETriggerEvent& TriggerEvent,
    const UInputAction* InputAction) const
{
    const bool bHasClass = ensureMsgf(InputID > INDEX_NONE, TEXT("Please ensure the Input ID is valid!"));
    return Super::CanHandle_Implementation(TriggerEvent, InputAction) && bHasClass;
}

UGameplayAbility* UInputHandler_AbilityInput::GetAbilityInstance(const UNinjaInputManagerComponent* Manager) const
{
	const TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = Manager->GetAbilitySystemComponent();
    
	if (ensureMsgf(IsValid(AbilitySystemComponent), TEXT("No ASC received from the Input Manager.")) &&
		ensureMsgf(InputID > INDEX_NONE, TEXT("The Input ID must be equal or greater than zero.")))
	{
		TArray<const FGameplayAbilitySpec*> Specs;
		AbilitySystemComponent->FindAllAbilitySpecsFromInputID(InputID, Specs);

		for (const FGameplayAbilitySpec* Spec : Specs)
		{
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

bool UInputHandler_AbilityInput::ActivateAbility(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
    const UInputAction* InputAction) const
{
    UAbilitySystemComponent* AbilitySystemComponent = Manager->GetAbilitySystemComponent();
    if (ensure(IsValid(AbilitySystemComponent)))
    {
        UE_LOG(LogNinjaInputHandler, Verbose, TEXT("[%s] Action %s is triggering an ability with ID %d."),
                *GetNameSafe(Manager->GetOwner()), *GetNameSafe(InputAction), InputID);

        AbilitySystemComponent->AbilityLocalInputPressed(InputID);
    	return true;
    }

	return false;
}

bool UInputHandler_AbilityInput::CancelAbility(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
    const UInputAction* InputAction) const
{
    return FNinjaInputHandlerHelpers::InterruptAbilityByInputID(Manager, InputAction, InputID);
}
