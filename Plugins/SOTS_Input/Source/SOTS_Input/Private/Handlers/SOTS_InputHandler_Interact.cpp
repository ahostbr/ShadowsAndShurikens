#include "Handlers/SOTS_InputHandler_Interact.h"

USOTS_InputHandler_Interact::USOTS_InputHandler_Interact()
{
    IntentTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Intent.Gameplay.Interact"), false);
}
