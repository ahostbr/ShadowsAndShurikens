// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_AbilityCancel.h"

#include "AbilitySystemComponent.h"
#include "Components/NinjaInputManagerComponent.h"

UInputHandler_AbilityCancel::UInputHandler_AbilityCancel()
{
	TriggerEvents.Add(ETriggerEvent::Triggered);
}

void UInputHandler_AbilityCancel::HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, const float ElapsedTime) const
{
	if (!CanCancelAbility(Manager, Value, InputAction, ElapsedTime))
	{
		return;
	}
	
	UAbilitySystemComponent* AbilitySystemComponent = Manager->GetAbilitySystemComponent();
	if (ensure(IsValid(AbilitySystemComponent)))
	{
		AbilitySystemComponent->LocalInputCancel();
	}
}

bool UInputHandler_AbilityCancel::CanCancelAbility_Implementation(UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const
{
	return true;
}
