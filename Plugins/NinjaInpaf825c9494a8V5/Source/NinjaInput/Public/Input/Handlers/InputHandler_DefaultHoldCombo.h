// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputHandler.h"
#include "InputHandler_DefaultHoldCombo.generated.h"

/**
 * Determines the activation role in the context of the combo. 
 */
UENUM(BlueprintType)
enum class EDefaultHoldComboActivationRole : uint8
{
	Started,
	Default,
	Hold,
	Combo,
	ComboDefault,
	ComboHold
};

/**
 * Determines how this activation should happen in the context of the combo.
 */
UENUM(BlueprintType)
enum class EDefaultHoldComboActivationType : uint8
{
	Undefined UMETA(Hidden),
	GameplayEvent,
	GameplayTag
};

/**
 * A context struct that can be used across the handler.
 * Meant to be provided by the "BuildContext" function.
 */
USTRUCT(BlueprintType)
struct NINJAINPUT_API FDefaultHoldComboState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Default Hold Combo State")
	bool bIsFirstComboAttack = true;

	UPROPERTY(BlueprintReadOnly, Category = "Default Hold Combo State")
	bool bIsInComboWindow = false;

	UPROPERTY(BlueprintReadOnly, Category = "Default Hold Combo State")
	bool bIsDefaultAbilityActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Default Hold Combo State")
	bool bIsHoldThresholdPassed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Default Hold Combo State")
	float TimeHeld = 0.f;
};

/**
 * An Input Handler that handles three ability scenarios from the same Input Action.
 *
 * 1. Default Activation: Triggers a default ability, when the input is quickly released.
 * 2. Hold to Activate: Triggers a different ability, like a "hold" attack.
 * 3. Combo Event: Sends a Gameplay Event to the active default ability.
 *
 * The accompanying input handler just requires a "Hold" trigger. Make sure to set the
 * trigger as "One Shot", otherwise this handler will keep trying to activate the same
 * ability each frame.
 */
UCLASS(meta = (DisplayName = "GAS: Default, Hold, Combo"))
class NINJAINPUT_API UInputHandler_DefaultHoldCombo : public UNinjaInputHandler
{
	
	GENERATED_BODY()

public:

	UInputHandler_DefaultHoldCombo();

	// -- Begin Input Handler implementation
	virtual void HandleStartedEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value, const UInputAction* InputAction) const override;
	virtual void HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const override;
	virtual void HandleCancelledEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const override;
	// -- End Input Handler implementation	

	/**
	 * Provides the Gameplay Activation Tag for a given role.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	FGameplayTag GetGameplayAbilityTagForRole(EDefaultHoldComboActivationRole Role) const;

	/**
	 * Provides the Gameplay Event Tag for a given role.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	FGameplayTag GetGameplayEventTagForRole(EDefaultHoldComboActivationRole Role) const;

	/**
	 * Determines the current state, based on the provided information.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	static FDefaultHoldComboState BuildDefaultHoldComboState(const UNinjaInputManagerComponent* Manager,
		float ElapsedTime, const UInputAction* InputAction, const UInputHandler_DefaultHoldCombo* Handler);
	
protected:

	// ----------
	// Activation of the "Start" role.
	// ----------
	
	/**
	 * Determines how the start ability activates.
	 * You can change this dynamically, via "GetStartActivationType".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType StartActivationType;
	
	/** Gameplay Tag used to activate an ability when the charge starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "StartActivationType == EDefaultHoldComboActivationType::GameplayTag"))
	FGameplayTag StartAbilityTag;

	/** Gameplay Event Tag used to activate an ability with the charge role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "StartActivationType == EDefaultHoldComboActivationType::GameplayEvent"))
	FGameplayTag StartEventTag;

	// ----------
	// Activation of the "Default" role.
	// ----------

	/**
	 * Determines how the default ability activates.
	 * You can change this dynamically, via "GetDefaultActivationType".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType DefaultActivationType;
	
	/** Gameplay Tag used to activate the default ability. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "DefaultActivationType == EDefaultHoldComboActivationType::GameplayTag"))
	FGameplayTag DefaultAbilityTag;

	/** Gameplay Event Tag used to activate an ability with the default role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "DefaultActivationType == EDefaultHoldComboActivationType::GameplayEvent"))
	FGameplayTag DefaultEventTag;

	// ----------
	// Activation of the "Hold" role.
	// ----------

	/**
	 * Determines how the hold ability activates.
	 * You can change this dynamically, via "GetHoldActivationType".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType HoldActivationType;
	
	/** Gameplay Tag used to activate the ability, when the Hold trigger activates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "HoldActivationType == EDefaultHoldComboActivationType::GameplayTag"))
	FGameplayTag HoldAbilityTag;

	/** Gameplay Event Tag used to activate an ability with the hold role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "HoldActivationType == EDefaultHoldComboActivationType::GameplayEvent"))
	FGameplayTag HoldEventTag;
	
	// ----------
	// Activation of the "Combo" role.
	// ----------

	/**
	 * Determines how the combo ability activates.
	 * You can change this dynamically, via "GetComboActivationType".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType ComboActivationType;

	/** Gameplay Tag used to activate the ability, when the Combo trigger activates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "ComboActivationType == EDefaultHoldComboActivationType::GameplayTag"))
	FGameplayTag ComboAbilityTag;
	
	/** Gameplay Event Tag used to activate an ability with the combo role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "ComboActivationType == EDefaultHoldComboActivationType::GameplayEvent"))
	FGameplayTag ComboEventTag;

	// ----------
	// Activation of the "ComboHold" role.
	// ----------

	/**
	 * Determines how the combo ability activates.
	 * You can change this dynamically, via "GetComboHoldActivationType".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType ComboHoldActivationType;

	/** Gameplay Tag used to activate the ability, when the Combo trigger activates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "ComboHoldActivationType == EDefaultHoldComboActivationType::GameplayTag"))
	FGameplayTag ComboHoldAbilityTag;
	
	/** Gameplay Event Tag used to activate an ability with the combo role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "ComboHoldActivationType == EDefaultHoldComboActivationType::GameplayEvent"))
	FGameplayTag ComboHoldEventTag;

	// ----------
	// Activation of the "ComboDefault" role.
	// ----------

	/**
	 * Determines how the combo ability activates.
	 * You can change this dynamically, via "GetComboDefaultActivationType".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType ComboDefaultActivationType;

	/** Gameplay Tag used to activate the ability, when the Combo trigger activates. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "ComboDefaultActivationType == EDefaultHoldComboActivationType::GameplayTag"))
	FGameplayTag ComboDefaultAbilityTag;
	
	/** Gameplay Event Tag used to activate an ability with the combo role. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default, Hold, Combo", meta = (EditCondition = "ComboDefaultActivationType == EDefaultHoldComboActivationType::GameplayEvent"))
	FGameplayTag ComboDefaultEventTag;
	
	/**
	 * Provides the activation type for the Start ability.
	 * 
	 * Can provide the default property value, or something else, based on the incoming values
	 * that were received by the function, such as a different Input Action, Value or something
	 * else entirely in the owner.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType GetStartActivationType(const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const;

	/**
	 * Provides the activation type for the Default ability.
	 * 
	 * Can provide the default property value, or something else, based on the incoming values
	 * that were received by the function, such as a different Input Action, Value or something
	 * else entirely in the owner.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType GetDefaultActivationType(const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const;

	/**
	 * Provides the activation type for the Hold ability.
	 * 
	 * Can provide the default property value, or something else, based on the incoming values
	 * that were received by the function, such as a different Input Action, Value or something
	 * else entirely in the owner.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType GetHoldActivationType(const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const;

	/**
	 * Provides the activation type for the Combo ability.
	 * 
	 * Can provide the default property value, or something else, based on the incoming values
	 * that were received by the function, such as a different Input Action, Value or something
	 * else entirely in the owner.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType GetComboActivationType(const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const;

	/**
	 * Provides the activation type for the Combo + Hold ability.
	 * 
	 * Can provide the default property value, or something else, based on the incoming values
	 * that were received by the function, such as a different Input Action, Value or something
	 * else entirely in the owner.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType GetComboHoldActivationType(const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const;

	/**
	 * Provides the activation type for the Combo + Default ability.
	 * 
	 * Can provide the default property value, or something else, based on the incoming values
	 * that were received by the function, such as a different Input Action, Value or something
	 * else entirely in the owner.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Default, Hold, Combo")
	EDefaultHoldComboActivationType GetComboDefaultActivationType(const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const;
	
	/**
	 * Evaluates the context and attempts to activate the ability for the role.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	void EvaluateAndActivateAbility(EDefaultHoldComboActivationRole Role, UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction, const FDefaultHoldComboState& State) const;

	/**
	 * Performs the actual activation logic, based on the activation method set for a role.
	 */
	void ActivateAbility(EDefaultHoldComboActivationRole Role, const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction, float TimeHeld = 0.f) const;
	
	/**
	 * Sends a Gameplay Event to the active default ability.
	 * Useful to advance combos, if you are using Ninja Combat.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	virtual void SendGameplayEvent(FGameplayTag EventTag, const UNinjaInputManagerComponent* Manager, 
		const FInputActionValue& Value, const UInputAction* InputAction, float Magnitude = 0.f) const;

	/**
	 * Checks if the default ability is active.
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	bool IsDefaultAbilityActive(const UNinjaInputManagerComponent* Manager) const;

	/**
	 * Checks if this is the first attack in a combo.
	 *
	 * By default, this input handler does not have a way to actually check this, so it
	 * will always consider to be in the first attack of a combo, until you connect this
	 * function to another system, such as Ninja Combat.
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	bool IsFirstComboAttack(const UNinjaInputManagerComponent* Manager) const;

	/**
	 * Checks if the player is in a combo window, to avoid unnecessary gameplay events. 
	 *
	 * By default, this input handler does not have a way to actually check this, so it
	 * will always consider to be in a combo window, until you connect this function to
	 * another system, such as Ninja Combat.
	 */
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	bool IsInComboWindow(const UNinjaInputManagerComponent* Manager) const;
	
	/**
	 * Provides the activation type for a given role.
	 */
	UFUNCTION(BlueprintPure, Category = "Ninja Input|Handlers|Default, Hold, Combo")
	EDefaultHoldComboActivationType GetActivationTypeForRole(EDefaultHoldComboActivationRole Role, const UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction) const;
};


