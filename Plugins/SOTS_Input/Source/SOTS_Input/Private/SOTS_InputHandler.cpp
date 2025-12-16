#include "SOTS_InputHandler.h"

#include "InputAction.h"
#include "InputActionValue.h"
#include "InputActionInstance.h"
#include "SOTS_InputRouterComponent.h"

bool USOTS_InputHandler::CanHandle(const UInputAction* Action, ETriggerEvent Event) const
{
    if (!Action)
    {
        return false;
    }

    if (InputActions.Num() == 0 || TriggerEvents.Num() == 0)
    {
        return false;
    }

    const bool bMatchesAction = InputActions.Contains(Action);
    const bool bMatchesEvent = TriggerEvents.Contains(Event);
    return bMatchesAction && bMatchesEvent;
}

bool USOTS_InputHandler::ShouldBuffer(const UInputAction* Action, ETriggerEvent Event, const FGameplayTag& OpenChannel) const
{
    return bAllowBuffering && BufferChannel.IsValid() && BufferChannel == OpenChannel;
}

void USOTS_InputHandler::HandleInput_Implementation(USOTS_InputRouterComponent* /*Router*/, const FInputActionInstance& /*Instance*/, ETriggerEvent /*TriggerEvent*/)
{
    // Default no-op; override in BP or C++.
}

void USOTS_InputHandler::HandleBufferedInput_Implementation(USOTS_InputRouterComponent* /*Router*/, const UInputAction* /*Action*/, ETriggerEvent /*TriggerEvent*/, FInputActionValue /*Value*/)
{
    // Default no-op; override in BP or C++.
}

void USOTS_InputHandler::OnActivated(USOTS_InputRouterComponent* /*Router*/)
{
}

void USOTS_InputHandler::OnDeactivated(USOTS_InputRouterComponent* /*Router*/)
{
}
