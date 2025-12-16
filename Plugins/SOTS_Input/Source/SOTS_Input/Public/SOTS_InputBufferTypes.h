#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "SOTS_InputBufferTypes.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct SOTS_INPUT_API FSOTS_BufferedInputEvent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<const UInputAction> Action = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETriggerEvent TriggerEvent = ETriggerEvent::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInputActionValue Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TimeSeconds = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag Channel;
};

/** Wrapper to allow storing buffered event arrays as TMap values. */
USTRUCT(BlueprintType)
struct SOTS_INPUT_API FSOTS_BufferedInputEventArray
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FSOTS_BufferedInputEvent> Events;
};
