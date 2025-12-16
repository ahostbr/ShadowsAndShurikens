#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "SOTS_InputHandler.generated.h"

struct FInputActionInstance;
class USOTS_InputRouterComponent;

// STABLE API: used by SOTS_UI and gameplay systems
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class SOTS_INPUT_API USOTS_InputHandler : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TArray<TObjectPtr<const UInputAction>> InputActions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TArray<ETriggerEvent> TriggerEvents;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffer")
    bool bAllowBuffering = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buffer")
    FGameplayTag BufferChannel;

    UFUNCTION(BlueprintCallable, Category = "Input")
    bool CanHandle(const UInputAction* Action, ETriggerEvent Event) const;

    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual bool ShouldBuffer(const UInputAction* Action, ETriggerEvent Event, const FGameplayTag& OpenChannel) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Input")
    void HandleInput(USOTS_InputRouterComponent* Router, const FInputActionInstance& Instance, ETriggerEvent TriggerEvent);

    UFUNCTION(BlueprintNativeEvent, Category = "Input")
    void HandleBufferedInput(USOTS_InputRouterComponent* Router, const UInputAction* Action, ETriggerEvent TriggerEvent, FInputActionValue Value);

    virtual void OnActivated(USOTS_InputRouterComponent* Router);
    virtual void OnDeactivated(USOTS_InputRouterComponent* Router);
};
