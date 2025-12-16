#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "SOTS_InputBindingTypes.generated.h"

USTRUCT()
struct FSOTS_InputBindingKey
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<const UInputAction> Action = nullptr;

    UPROPERTY()
    ETriggerEvent TriggerEvent = ETriggerEvent::None;

    bool operator==(const FSOTS_InputBindingKey& Other) const
    {
        return Action == Other.Action && TriggerEvent == Other.TriggerEvent;
    }

    FString ToString() const
    {
        const FString ActionName = Action ? Action->GetName() : TEXT("None");
        return FString::Printf(TEXT("%s:%d"), *ActionName, static_cast<uint8>(TriggerEvent));
    }
};

inline uint32 GetTypeHash(const FSOTS_InputBindingKey& BindingKey)
{
    return HashCombine(::GetTypeHash(BindingKey.Action), ::GetTypeHash(static_cast<uint8>(BindingKey.TriggerEvent)));
}
