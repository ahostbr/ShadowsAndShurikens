#include "Handlers/SOTS_InputHandler_IntentTag.h"

#include "InputAction.h"
#include "SOTS_InputRouterComponent.h"

void USOTS_InputHandler_IntentTag::HandleInput_Implementation(USOTS_InputRouterComponent* Router, const FInputActionInstance& Instance, ETriggerEvent TriggerEvent)
{
    if (!Router)
    {
        return;
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
