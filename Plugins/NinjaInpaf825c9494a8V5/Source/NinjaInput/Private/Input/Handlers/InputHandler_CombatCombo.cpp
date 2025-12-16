// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_CombatCombo.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "NinjaInputLogs.h"
#include "NinjaInputTags.h"
#include "Components/NinjaInputManagerComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UInputHandler_CombatCombo::UInputHandler_CombatCombo()
{
	ActivationTags = FGameplayTagContainer::EmptyContainer;
	AdvancementTag = FGameplayTag::EmptyTag;
	ComboStateTag = Tag_Input_State_ComboWindow;
	TriggerEvents.Add(ETriggerEvent::Triggered);
}

void UInputHandler_CombatCombo::HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value, const UInputAction* InputAction, const float ElapsedTime) const
{
	const AActor* Pawn = IsValid(Manager) ? Manager->GetPawn() : nullptr;
	const UAbilitySystemComponent* AbilityComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);

	if (!IsValid(AbilityComponent))
	{
		return;
	}

	if (AbilityComponent->HasMatchingGameplayTag(ComboStateTag))
	{
		UE_LOG(LogNinjaInputCombatComboHandler, Verbose, TEXT("Advancing combo from input %s."), *GetNameSafe(InputAction));
		AdvanceCombo(Manager, InputAction);
	}
	else
	{
		UE_LOG(LogNinjaInputCombatComboHandler, Verbose, TEXT("Starting combo from input %s."), *GetNameSafe(InputAction));
		StartCombo(Manager);
	}
}

void UInputHandler_CombatCombo::StartCombo(const UNinjaInputManagerComponent* Manager) const
{
	if (!bHandlesActivation || ActivationTags.IsEmpty())
	{
		return;	
	}

	const AActor* Pawn = IsValid(Manager) ? Manager->GetPawn() : nullptr;
	UAbilitySystemComponent* AbilityComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
	if (!IsValid(AbilityComponent))
	{
		return;
	}

	AbilityComponent->TryActivateAbilitiesByTag(ActivationTags);	
}

// ReSharper disable once CppExpressionWithoutSideEffects
void UInputHandler_CombatCombo::AdvanceCombo(const UNinjaInputManagerComponent* Manager, const UInputAction* InputAction) const
{
	if (!IsValid(Manager))
	{
		return;
	}

	const FInputActionValue Value = FInputActionValue(true);
	Manager->SendGameplayEventToOwnerWithMagnitude(AdvancementTag, Value, InputAction);	
}