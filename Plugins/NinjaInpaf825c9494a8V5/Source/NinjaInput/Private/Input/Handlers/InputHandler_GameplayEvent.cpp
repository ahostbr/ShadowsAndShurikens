// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_GameplayEvent.h"

#include "AbilitySystemComponent.h"
#include "NinjaInputHandlerHelpers.h"
#include "Components/NinjaInputManagerComponent.h"

UInputHandler_GameplayEvent::UInputHandler_GameplayEvent()
{
	bOverrideMagnitude = false;
	Magnitude = 0.f;
	EventTag = FGameplayTag::EmptyTag;
	TriggerEvents.Add(ETriggerEvent::Triggered);
}

void UInputHandler_GameplayEvent::HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const
{
	SendGameplayEvent(Manager, EventTag, Value, InputAction);
}

bool UInputHandler_GameplayEvent::CanSendGameplayEvent_Implementation(UNinjaInputManagerComponent* Manager,
	FGameplayTag GameplayEventTag, const FInputActionValue& Value, const UInputAction* InputAction) const
{
	return true;
}

int32 UInputHandler_GameplayEvent::SendGameplayEvent(UNinjaInputManagerComponent* Manager,
	const FGameplayTag GameplayEventTag, const FInputActionValue& Value, const UInputAction* InputAction) const
{
	if (!CanSendGameplayEvent(Manager, GameplayEventTag, Value, InputAction))
	{
		return 0;
	}
	
	const FGameplayTagContainer InstigatorTags = GetInstigatorTags();
	const FGameplayTagContainer TargetTags = GetTargetTags();
	const UObject* OptionalObject2 = GetOptionalObject2();
	const float FinalMagnitude = bOverrideMagnitude ? Magnitude : Value.GetMagnitude();
	
	return FNinjaInputHandlerHelpers::SendGameplayEvent(Manager, GameplayEventTag, Value, InputAction, FinalMagnitude, InstigatorTags, TargetTags, OptionalObject2);	
}

FGameplayTagContainer UInputHandler_GameplayEvent::GetInstigatorTags_Implementation() const
{
	return FGameplayTagContainer();
}

FGameplayTagContainer UInputHandler_GameplayEvent::GetTargetTags_Implementation() const
{
	return FGameplayTagContainer();
}

const UObject* UInputHandler_GameplayEvent::GetOptionalObject2_Implementation() const
{
	return nullptr;
}