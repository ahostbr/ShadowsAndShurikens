// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputHandler.h"
#include "InputHandler_CharacterCrouch.generated.h"

/**
 * Replicates the original Crouch setup for Character owners.
 *
 * Avoid mixing default Character actions, such as this one, when you are using the Gameplay
 * Ability System and try to consolidate every ability to be triggered by GAS.
 */
UCLASS(meta = (DisplayName = "Character: Crouch"))
class NINJAINPUT_API UInputHandler_CharacterCrouch : public UNinjaInputHandler
{
	
	GENERATED_BODY()

public:

	UInputHandler_CharacterCrouch();

	/**
	 * Checks if the character is allowed to crouch.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Input Handler|Crouch")	
	bool CanCrouch(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value) const;

	/**
	 * Evaluates input conditions to determine if the character should uncrouch.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Input Handler|Crouch")	
	bool ShouldUncrouch(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value) const;
	
protected:

	/** Defines if the crouching is toggled or momentary. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Crouch")
	bool bToggle;
	
	/** Gameplay tags that, if present, will block the ability of crouching/un-crouching. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Crouch")
	FGameplayTagContainer BlockCrouchTags;
	
	virtual void HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
		const UInputAction* InputAction, float ElapsedTime) const override;

};
