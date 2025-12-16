// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NinjaInputHandler.h"
#include "InputHandler_GameplayEvent.generated.h"

/**
 * Triggers a Gameplay Event.
 *
 * The Value and Action are sent in the Gameplay Event as Magnitude and the first Additional Object,
 * so they can be accessed by Gameplay Abilities interested in those values.
 */
UCLASS(DisplayName = "GAS: Send Gameplay Event")
class NINJAINPUT_API UInputHandler_GameplayEvent : public UNinjaInputHandler
{
	
	GENERATED_BODY()

public:

	UInputHandler_GameplayEvent();

protected:

	/** Gameplay Tag representing the event to broadcast. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Event")
	FGameplayTag EventTag;

	/** Determines if the default magnitude should be overriden. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Event", meta = (InlineEditConditionToggle))
	bool bOverrideMagnitude;

	/** Value to override the default magnitude. By default, the Input Value magnitude is used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Event", meta = (EditCondition = "bOverrideMagnitude"))
	float Magnitude;
	
	virtual void HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager,
		const FInputActionValue& Value, const UInputAction* InputAction, float ElapsedTime) const override;

	/** Tags adds as Instigator Tags in the payload. */
	UFUNCTION(BlueprintNativeEvent, Category = "Gameplay Event")
	FGameplayTagContainer GetInstigatorTags() const;

	/** Tags adds as Target Tags in the payload. */
	UFUNCTION(BlueprintNativeEvent, Category = "Gameplay Event")
	FGameplayTagContainer GetTargetTags() const;

	/** Provides a secondary optional object. */
	UFUNCTION(BlueprintNativeEvent, Category = "Gameplay Event")
	const UObject* GetOptionalObject2() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Gameplay Event")
	bool CanSendGameplayEvent(UNinjaInputManagerComponent* Manager, FGameplayTag GameplayEventTag,
		const FInputActionValue& Value, const UInputAction* InputAction) const;
	
	/**
	 * Sends a gameplay event using the internal functions to build the payload.
	 *
	 * @param Manager
	 *		Input Manager that is responsible for this handler execution.
	 *		
	 * @param GameplayEventTag
	 *		Actual tag to use. This would usually be the one set in the instance, but we
	 *		want to have a flexibility to send multiple events from the same handler.
	 *
	 * @param Value
	 *		Incoming value that will be used as the event magnitude.
	 *
	 * @param InputAction
	 *		Input Action used as te first Optional Object.
	 *		
	 * @return
	 *		Number of activations from the Gameplay Event.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ninja Input|Handlers|Send Gameplay Event", meta = (ReturnDisplayName = "Activations"))
	int32 SendGameplayEvent(UNinjaInputManagerComponent* Manager, FGameplayTag GameplayEventTag,
		const FInputActionValue& Value, const UInputAction* InputAction) const;
	
};
