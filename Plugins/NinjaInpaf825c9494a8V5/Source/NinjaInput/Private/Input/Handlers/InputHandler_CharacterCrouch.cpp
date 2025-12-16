// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_CharacterCrouch.h"

#include "InputAction.h"
#include "NinjaInputSettings.h"
#include "NinjaInputTags.h"
#include "Components/NinjaInputManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"

UInputHandler_CharacterCrouch::UInputHandler_CharacterCrouch()
{
	bToggle = true;
	BlockCrouchTags = FGameplayTagContainer::EmptyContainer;
	BlockCrouchTags.AddTagFast(Tag_Input_Block_Movement);
	
	TriggerEvents.Add(ETriggerEvent::Triggered);

	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchToggleInputActionRef(TEXT("/Script/EnhancedInput.InputAction'/NinjaInput/Input/IA_NI_Crouch_Toggle.IA_NI_Crouch_Toggle'"));
	if (CrouchToggleInputActionRef.Succeeded())
	{
		const TObjectPtr<UInputAction> InputAction = CrouchToggleInputActionRef.Object;
		InputActions.AddUnique(InputAction);
	}
}

bool UInputHandler_CharacterCrouch::CanCrouch_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value) const
{
	if (HasAnyTags(Manager, BlockCrouchTags))
	{
		return false;	
	}

	const ACharacter* OwningCharacter = Cast<ACharacter>(Manager->GetPawn());
	if (!OwningCharacter || !OwningCharacter->GetCharacterMovement())
	{
		return false;
	}

	if (!OwningCharacter->GetCharacterMovement()->CanEverCrouch() || OwningCharacter->GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	return true;
}

bool UInputHandler_CharacterCrouch::ShouldUncrouch_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value) const
{
	const ACharacter* OwningCharacter = Cast<ACharacter>(Manager->GetPawn());
	if (!OwningCharacter || !OwningCharacter->GetCharacterMovement())
	{
		return false;
	}

	// The toggle/momentary flag is handled also taking the Input Action into consideration:
	//
	// TOGGLE:
	//		The handler is set to "toggle", and we received a "true" value from the handler, which is most
	//		likely using a "Pressed" trigger, that was pressed before and the character is crouching.
	//
	// MOMENTARY:
	//		The handler is not set to "toggle", and we received a "false" value from the handler, which is
	//		most likely using both the "Pressed" and "Released" triggers in conjunction.
	//
	return OwningCharacter->bIsCrouched && ((bToggle && Value.Get<bool>()) || (!bToggle && !Value.Get<bool>()));
}

void UInputHandler_CharacterCrouch::HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const
{
	ACharacter* OwningCharacter = Cast<ACharacter>(Manager->GetPawn());
	if (IsValid(OwningCharacter))
	{
		if (ShouldUncrouch(Manager, Value))
		{
			OwningCharacter->UnCrouch();
		}
		else if (CanCrouch(Manager, Value))
		{
			OwningCharacter->Crouch();
		}
	}
}
