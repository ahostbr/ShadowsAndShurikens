#include "Handlers/SOTS_InputHandler_IntentTag.h"

#include "InputAction.h"
#include "SOTS_InputBufferComponent.h"
#include "SOTS_InputRouterComponent.h"

void USOTS_InputHandler_IntentTag::HandleInput_Implementation(USOTS_InputRouterComponent* Router, const FInputActionInstance& Instance, ETriggerEvent TriggerEvent)
{
    if (!Router)
    {
        return;
    }

    if (USOTS_InputBufferComponent* Buffer = Router->GetOrFindBufferComponent())
    {
        static const FGameplayTag ExecutionChannel = FGameplayTag::RequestGameplayTag(TEXT("Input.Buffer.Channel.Execution"), false);
        static const FGameplayTag VanishChannel = FGameplayTag::RequestGameplayTag(TEXT("Input.Buffer.Channel.Vanish"), false);

        bool bBuffered = false;
        Buffer->TryBufferIntent(ExecutionChannel, IntentTag, bBuffered);
        Buffer->TryBufferIntent(VanishChannel, IntentTag, bBuffered);
    }

    Router->BroadcastIntent(IntentTag, TriggerEvent, Instance.GetValue());
}

void USOTS_InputHandler_IntentTag::HandleBufferedInput_Implementation(USOTS_InputRouterComponent* Router, const UInputAction* /*Action*/, ETriggerEvent TriggerEvent, FInputActionValue Value)
{
    if (!Router)
    {
        return;
    }

    Router->BroadcastIntent(IntentTag, TriggerEvent, Value);
}
