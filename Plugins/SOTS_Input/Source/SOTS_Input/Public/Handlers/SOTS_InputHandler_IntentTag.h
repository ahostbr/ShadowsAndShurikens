#pragma once

#include "CoreMinimal.h"
#include "SOTS_InputHandler.h"
#include "SOTS_InputHandler_IntentTag.generated.h"

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class SOTS_INPUT_API USOTS_InputHandler_IntentTag : public USOTS_InputHandler
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intent")
    FGameplayTag IntentTag;

    virtual void HandleInput_Implementation(USOTS_InputRouterComponent* Router, const FInputActionInstance& Instance, ETriggerEvent TriggerEvent) override;
    virtual void HandleBufferedInput_Implementation(USOTS_InputRouterComponent* Router, const UInputAction* Action, ETriggerEvent TriggerEvent, FInputActionValue Value) override;
};
