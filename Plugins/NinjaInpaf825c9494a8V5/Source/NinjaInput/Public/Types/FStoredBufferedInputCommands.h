// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "FBufferedInputCommand.h"
#include "FStoredBufferedInputCommands.generated.h"

/**
 * A list of buffered commands consolidated and ready to be used.
 */
USTRUCT(BlueprintType)
struct FStoredBufferedInputCommands
{
	
	GENERATED_BODY()

	/** The main Input Action that will trigger these commands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Buffered Input Commands")
	TObjectPtr<const UInputAction> InputAction = nullptr;

	/** Commands that will be triggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stored Buffered Input Commands")
	TArray<FBufferedInputCommand> InputCommandsForAction;

	FStoredBufferedInputCommands()
	{
	}

	FStoredBufferedInputCommands(const TArray<FBufferedInputCommand>& InInputCommandsForAction)
	{
		if (!InInputCommandsForAction.IsEmpty())
		{
			InputAction = InInputCommandsForAction[0].GetSourceAction();
			InputCommandsForAction.Append(InInputCommandsForAction);
		}
	}

	/** Checks if this struct contains valid data. */
	bool IsValid() const
	{
		return InputAction != nullptr && InputCommandsForAction.Num() > 0;
	}

	/** Counts how many commands were stored in this buffer. */
	int32 Num() const
	{
		return InputCommandsForAction.Num();
	}
};