// Ninja Bear Studio Inc., all rights reserved.
#include "Input/Handlers/InputHandler_DefaultHoldCombo.h"

#include "AbilitySystemComponent.h"
#include "NinjaInputFunctionLibrary.h"
#include "Components/NinjaInputManagerComponent.h"

UInputHandler_DefaultHoldCombo::UInputHandler_DefaultHoldCombo()
{
	StartActivationType = EDefaultHoldComboActivationType::GameplayTag;
	DefaultActivationType = EDefaultHoldComboActivationType::GameplayTag;
	HoldActivationType = EDefaultHoldComboActivationType::GameplayTag;
	ComboActivationType = EDefaultHoldComboActivationType::GameplayEvent;
	ComboHoldActivationType = EDefaultHoldComboActivationType::GameplayEvent;
	ComboDefaultActivationType = EDefaultHoldComboActivationType::GameplayEvent;
	
	TriggerEvents.Add(ETriggerEvent::Started);
	TriggerEvents.Add(ETriggerEvent::Triggered);
	TriggerEvents.Add(ETriggerEvent::Canceled);
}

void UInputHandler_DefaultHoldCombo::HandleStartedEvent_Implementation(UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction) const
{
	static constexpr float ElapsedTime = 0.f;
	const FDefaultHoldComboState State = BuildDefaultHoldComboState(Manager, ElapsedTime, InputAction, this);
	
	if (!State.bIsFirstComboAttack && State.bIsInComboWindow)
	{
		// Execute the next combo attack.
		EvaluateAndActivateAbility(EDefaultHoldComboActivationRole::Combo, Manager, Value, InputAction, State);
		return;
	}

	EvaluateAndActivateAbility(EDefaultHoldComboActivationRole::Started, Manager, Value, InputAction, State);	
}

void UInputHandler_DefaultHoldCombo::HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, const float ElapsedTime) const
{
	const FDefaultHoldComboState State = BuildDefaultHoldComboState(Manager, ElapsedTime, InputAction, this);
	if (!State.bIsFirstComboAttack && State.bIsDefaultAbilityActive)
	{
		// Trigger a hold ability or event inside the combo.
		EvaluateAndActivateAbility(EDefaultHoldComboActivationRole::ComboHold, Manager, Value, InputAction, State);
		return;
	}

	// First attack logic.
	const EDefaultHoldComboActivationRole Role = State.bIsHoldThresholdPassed
		? EDefaultHoldComboActivationRole::Hold
		: EDefaultHoldComboActivationRole::Default;

	EvaluateAndActivateAbility(Role, Manager, Value, InputAction, State);
}

void UInputHandler_DefaultHoldCombo::HandleCancelledEvent_Implementation(UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, const float ElapsedTime) const
{
	const FDefaultHoldComboState State = BuildDefaultHoldComboState(Manager, ElapsedTime, InputAction, this);
	if (!State.bIsFirstComboAttack && State.bIsDefaultAbilityActive)
	{
		// Trigger the usual attack in the combo.
		EvaluateAndActivateAbility(EDefaultHoldComboActivationRole::ComboDefault, Manager, Value, InputAction, State);
	}
	else
	{
		// Tap release (or cancel after hold).
		EvaluateAndActivateAbility(EDefaultHoldComboActivationRole::Default, Manager, Value, InputAction, State);
	}	
}

void UInputHandler_DefaultHoldCombo::EvaluateAndActivateAbility(const EDefaultHoldComboActivationRole Role, UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, const FDefaultHoldComboState& State) const
{
	const float TimeHeld = State.TimeHeld;
	EDefaultHoldComboActivationRole ActualRole = Role;
	
	if (Role == EDefaultHoldComboActivationRole::Default)
	{
		// Default role requires additional context: are we in a combo?
		if (!State.bIsFirstComboAttack && State.bIsDefaultAbilityActive)
		{
			// Pressing again during an active combo.
			ActualRole = EDefaultHoldComboActivationRole::Combo;
		}
	}
	
	UE_LOG(LogNinjaInputHandler, VeryVerbose,
	TEXT("Resolved activation role: Input='%s' -> Actual='%s'"),
		*UEnum::GetValueAsString(Role), *UEnum::GetValueAsString(ActualRole));
	
	// All other roles are directly resolved.
	ActivateAbility(ActualRole, Manager, Value, InputAction, TimeHeld);
}

void UInputHandler_DefaultHoldCombo::ActivateAbility(const EDefaultHoldComboActivationRole Role, const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, const float TimeHeld) const
{
	static const UEnum* RoleEnum = StaticEnum<EDefaultHoldComboActivationRole>();
	
	const EDefaultHoldComboActivationType ActivationType = GetActivationTypeForRole(Role, Manager, Value, InputAction);
	if (ActivationType == EDefaultHoldComboActivationType::Undefined)
	{
		UE_LOG(LogNinjaInputHandler, Warning,
			TEXT("Input Handler '%s' has Undefined activation type for role '%s'. No action was performed."),
			*GetNameSafe(this),
			*RoleEnum->GetValueAsName(Role).ToString());

		return; 
	}

	if (ActivationType == EDefaultHoldComboActivationType::GameplayEvent)
	{
		const FGameplayTag EventTag = GetGameplayEventTagForRole(Role);
		if (EventTag.IsValid())
		{
			UE_LOG(LogNinjaInputHandler, Verbose,
				TEXT("Sending gameplay event '%s' for role '%s'."),
				*EventTag.ToString(), *RoleEnum->GetValueAsName(Role).ToString());

			SendGameplayEvent(EventTag, Manager, Value, InputAction, TimeHeld);
		}
	}
	else if (ActivationType == EDefaultHoldComboActivationType::GameplayTag)
	{
		const FGameplayTag AbilityTag = GetGameplayAbilityTagForRole(Role);
		UAbilitySystemComponent* AbilitySystemComponent = Manager->GetAbilitySystemComponent();

		if (AbilityTag.IsValid() && IsValid(AbilitySystemComponent))
		{
			UE_LOG(LogNinjaInputHandler, Verbose,
				TEXT("Activating ability tag '%s' for role '%s'."),
				*AbilityTag.ToString(), *RoleEnum->GetValueAsName(Role).ToString());

			const FGameplayTagContainer TagContainer(AbilityTag);
			AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
		}
	}	
}

void UInputHandler_DefaultHoldCombo::SendGameplayEvent(const FGameplayTag EventTag, const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction, const float Magnitude) const
{
	if (!EventTag.IsValid() || !IsValid(Manager))
	{
		return;
	}

	static constexpr bool bTriggerEventLocally = true;
	static constexpr bool bTriggerEventOnServer = true;

	// ReSharper disable once CppExpressionWithoutSideEffects
	Manager->SendGameplayEventToOwnerWithMagnitude(EventTag, Value, InputAction, Magnitude,
		bTriggerEventLocally, bTriggerEventOnServer);	
}

bool UInputHandler_DefaultHoldCombo::IsDefaultAbilityActive_Implementation(const UNinjaInputManagerComponent* Manager) const
{
	const UAbilitySystemComponent* AbilitySystemComponent = Manager->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent) || !DefaultAbilityTag.IsValid())
	{
		return false;
	}

	TArray<FGameplayAbilitySpecHandle> MatchingHandles;
	const FGameplayTagContainer TagContainer(DefaultAbilityTag);
	AbilitySystemComponent->FindAllAbilitiesWithTags(MatchingHandles, TagContainer);

	for (const FGameplayAbilitySpecHandle& Handle : MatchingHandles)
	{
		const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (Spec && Spec->IsActive())
		{
			UE_LOG(LogNinjaInputHandler, VeryVerbose,
				TEXT("Default ability '%s' is active."),
				*DefaultAbilityTag.ToString());

			return true;
		}
	}

	UE_LOG(LogNinjaInputHandler, VeryVerbose,
		TEXT("No default ability active for tag '%s'."),
		*DefaultAbilityTag.ToString());
	
	return false;
}

bool UInputHandler_DefaultHoldCombo::IsFirstComboAttack_Implementation(const UNinjaInputManagerComponent* Manager) const
{
	return true;
}

bool UInputHandler_DefaultHoldCombo::IsInComboWindow_Implementation(const UNinjaInputManagerComponent* Manager) const
{
	return true;
}

FGameplayTag UInputHandler_DefaultHoldCombo::GetGameplayAbilityTagForRole(const EDefaultHoldComboActivationRole Role) const
{
	switch (Role)
	{
		case EDefaultHoldComboActivationRole::Started: return StartAbilityTag;
		case EDefaultHoldComboActivationRole::Default: return DefaultAbilityTag;
		case EDefaultHoldComboActivationRole::Hold: return HoldAbilityTag;
		case EDefaultHoldComboActivationRole::Combo: return ComboAbilityTag;
		case EDefaultHoldComboActivationRole::ComboHold: return ComboHoldAbilityTag;
		case EDefaultHoldComboActivationRole::ComboDefault: return ComboDefaultAbilityTag;		
		default: return FGameplayTag::EmptyTag;
	}
}

FGameplayTag UInputHandler_DefaultHoldCombo::GetGameplayEventTagForRole(const EDefaultHoldComboActivationRole Role) const
{
	switch (Role)
	{
		case EDefaultHoldComboActivationRole::Started: return StartEventTag;
		case EDefaultHoldComboActivationRole::Default: return DefaultEventTag;
		case EDefaultHoldComboActivationRole::Hold: return HoldEventTag;
		case EDefaultHoldComboActivationRole::Combo: return ComboEventTag;
		case EDefaultHoldComboActivationRole::ComboHold: return ComboHoldEventTag;
		case EDefaultHoldComboActivationRole::ComboDefault: return ComboDefaultEventTag;
		default: return FGameplayTag::EmptyTag;
	}
}

EDefaultHoldComboActivationType UInputHandler_DefaultHoldCombo::GetActivationTypeForRole(const EDefaultHoldComboActivationRole Role,
	const UNinjaInputManagerComponent* Manager, const FInputActionValue& Value, const UInputAction* InputAction) const
{
	switch (Role)
	{
		case EDefaultHoldComboActivationRole::Started: return GetStartActivationType(Manager, Value, InputAction);
		case EDefaultHoldComboActivationRole::Default: return GetDefaultActivationType(Manager, Value, InputAction);
		case EDefaultHoldComboActivationRole::Hold: return GetHoldActivationType(Manager, Value, InputAction);
		case EDefaultHoldComboActivationRole::Combo: return GetComboActivationType(Manager, Value, InputAction);
		case EDefaultHoldComboActivationRole::ComboHold: return GetComboHoldActivationType(Manager, Value, InputAction);
		case EDefaultHoldComboActivationRole::ComboDefault: return GetComboDefaultActivationType(Manager, Value, InputAction);
		default: return EDefaultHoldComboActivationType::Undefined;
	}
}

EDefaultHoldComboActivationType UInputHandler_DefaultHoldCombo::GetStartActivationType_Implementation(const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction) const
{
	return StartActivationType;
}

EDefaultHoldComboActivationType UInputHandler_DefaultHoldCombo::GetDefaultActivationType_Implementation(const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction) const
{
	return DefaultActivationType;
}

EDefaultHoldComboActivationType UInputHandler_DefaultHoldCombo::GetHoldActivationType_Implementation(const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction) const
{
	return HoldActivationType;
}

EDefaultHoldComboActivationType UInputHandler_DefaultHoldCombo::GetComboActivationType_Implementation(const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction) const
{
	return ComboActivationType;
}

EDefaultHoldComboActivationType UInputHandler_DefaultHoldCombo::GetComboHoldActivationType_Implementation(const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction) const
{
	return ComboHoldActivationType;
}

EDefaultHoldComboActivationType UInputHandler_DefaultHoldCombo::GetComboDefaultActivationType_Implementation(const UNinjaInputManagerComponent* Manager,
	const FInputActionValue& Value, const UInputAction* InputAction) const
{
	return ComboDefaultActivationType;
}

FDefaultHoldComboState UInputHandler_DefaultHoldCombo::BuildDefaultHoldComboState(
	const UNinjaInputManagerComponent* Manager, const float ElapsedTime, const UInputAction* InputAction,
	const UInputHandler_DefaultHoldCombo* Handler)
{
	FDefaultHoldComboState State;
	State.TimeHeld = ElapsedTime;
	
	if (IsValid(Handler) && IsValid(Manager))
	{
		State.bIsFirstComboAttack = Handler->IsFirstComboAttack(Manager);
		State.bIsInComboWindow = Handler->IsInComboWindow(Manager);
		State.bIsDefaultAbilityActive = Handler->IsDefaultAbilityActive(Manager);
	}

	if (IsValid(InputAction))
	{
		const float HoldThreshold = UNinjaInputFunctionLibrary::GetHoldTimeThresholdForInputAction(InputAction);
		State.bIsHoldThresholdPassed = ElapsedTime >= HoldThreshold;
	}

	return State;
}