// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TraversalMovementInputInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UTraversalMovementInputInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * An API that can be plugged into traversal actions like jumping.
 */
class NINJAINPUT_API ITraversalMovementInputInterface
{
	
	GENERATED_BODY()

public:

	/**
	 * Attempts to do a traversal action of any kind.
	 *
	 * @param Value
	 *		Value received for the Input Action.
	 *		
	 * @param InputAction
	 *		The action that triggered this attempt. The same traversal action might be triggered from multiple Input
	 *		Actions, so this information can be used to trigger different behaviors.
	 *		
	 * @param TriggerEvent
	 *		The trigger event that originated this attempt. Different triggers, such as started or triggered, may lead
	 *		to different behaviors in the traversal action, so this can be taken into consideration.
	 * 
	 * @return
	 *		True if a traversal action was triggered, which overrides the default jump behavior. False otherwise.
	 *		Implementing this method is optional and if not deliberately implemented, it returns "false", meaning
	 *		no traversal jump action was executed.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Traversal Movement Input Interface")
	bool TryTraversalJumpAction(const FInputActionValue& Value, const UInputAction* InputAction, ETriggerEvent TriggerEvent);
	virtual bool TryTraversalJumpAction_Implementation(const FInputActionValue& Value, const UInputAction* InputAction, ETriggerEvent TriggerEvent) { return false; }
	
};