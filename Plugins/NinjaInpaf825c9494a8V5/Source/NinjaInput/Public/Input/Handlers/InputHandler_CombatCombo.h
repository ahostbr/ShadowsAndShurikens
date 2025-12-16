// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputHandler.h"
#include "InputHandler_CombatCombo.generated.h"

class UInputAction;

/**
 * This input handler will properly handle a Ninja Combat combo, with Activation/Advancement features.
 *
 * You can use this handler for both or either of the combo features. Activation will send the initial
 * set of tags to activate the Combo Ability (i.e. "Combat.Ability.Combo") and Advancement will check for an
 * active combo window and send the provided set of tags (i.e. "Combat.Event.Combo.Attack.Primary").
 *
 * Even though this is mostly designed considering Ninja Combat's input system, it's just a GAS based
 * check using gameplay tags, so if your combo system also follows that approach, this can be used too.
 */
UCLASS(DisplayName = "GAS: Combat Combo")
class NINJAINPUT_API UInputHandler_CombatCombo : public UNinjaInputHandler
{
	
	GENERATED_BODY()

public:

	UInputHandler_CombatCombo();

	/**
	 * Starts the combo using the provided Gameplay Tags.
	 */
	void StartCombo(const UNinjaInputManagerComponent* Manager) const;

	/**
	 * Advances the combo, creating the correct payload with the Input Action.
	 * Will check if the combo window is actually active.
	 */
	void AdvanceCombo(const UNinjaInputManagerComponent* Manager, const UInputAction* InputAction) const;
	
protected:

	/** Gameplay Tag that represents the combo state. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Combo")
	FGameplayTag ComboStateTag;
	
	/** Determines if this handler will take care of activation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Combo", meta = (InlineEditConditionToggle))
	bool bHandlesActivation = true;

	/** Gameplay Tags used to activate the combo ability. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Combo", meta = (EditCondition = bHandlesActivation))
	FGameplayTagContainer ActivationTags;
	
	/** Determines if this handler will take care of advancement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Combo", meta = (InlineEditConditionToggle))
	bool bHandlesAdvancement = true;
	
	/** Gameplay Tags used to advance the combo ability. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat Combo", meta = (EditCondition = bHandlesAdvancement))
	FGameplayTag AdvancementTag;

	// -- Begin InputHandler implementation
	virtual void HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const override;
	// -- End InputHandler implementation
	
};
