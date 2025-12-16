// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "NinjaInputHandler.h"
#include "InputHandler_ShowDebugString.generated.h"

/**
 * Shows a debug string in the editor UI.
 */
UCLASS(meta = (DisplayName = "Debug: Show String"))
class NINJAINPUT_API UInputHandler_ShowDebugString : public UNinjaInputHandler
{
	
	GENERATED_BODY()

public:

	UInputHandler_ShowDebugString();
	
protected:

	/**
	 * String that will be displayed in the user interface.
	 *
	 * Supports the following variables, that will be automatically replaced internally:
	 * - {value}: The magnitude of the incoming value.
	 * - {action}: Name of the Input Action that triggered the execution.
	 * - {pawn}: Pawn that owns the current Input Manager Component.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	FString DisplayString;

	/** Duration for the text. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	float Duration;

	/** Color used to render the text. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	FColor Color;
	
	virtual void HandleTriggeredEvent_Implementation(UNinjaInputManagerComponent* Manager, const FInputActionValue& Value,
		const UInputAction* InputAction, float ElapsedTime) const override;

};
