// Ninja Bear Studio Inc., all rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "FInputActionHandlerCache.generated.h"

class UNinjaInputHandler;

/** Cached structure for input handlers. */
USTRUCT(BlueprintType)
struct FInputActionHandlerCache
{
	
	GENERATED_BODY()

	TArray<TWeakObjectPtr<UNinjaInputHandler>> Started;
	TArray<TWeakObjectPtr<UNinjaInputHandler>> Triggered;
	TArray<TWeakObjectPtr<UNinjaInputHandler>> Ongoing;
	TArray<TWeakObjectPtr<UNinjaInputHandler>> Completed;
	TArray<TWeakObjectPtr<UNinjaInputHandler>> Canceled;

	FInputActionHandlerCache()
	{
	}
	
	TArray<TWeakObjectPtr<UNinjaInputHandler>>& GetHandlersForEvent(const ETriggerEvent Event)
	{
		switch (Event)
		{
			case ETriggerEvent::Started:   return Started;
			case ETriggerEvent::Triggered: return Triggered;
			case ETriggerEvent::Ongoing:   return Ongoing;
			case ETriggerEvent::Completed: return Completed;
			case ETriggerEvent::Canceled:  return Canceled;
			default:                       return Triggered;
		}
	}

	bool HasAny() const
	{
		return !Started.IsEmpty() || !Triggered.IsEmpty() || !Ongoing.IsEmpty() || !Completed.IsEmpty() || !Canceled.IsEmpty();
	}
};